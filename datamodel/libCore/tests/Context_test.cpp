/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Context.h"
#include "core/Queries.h"
#include "core/Handles.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"
#include "core/MeshCacheInterface.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/MockUserTypes.h"
#include "user_types/UserObjectFactory.h"
#include "testing/TestUtil.h"

#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::user_types;

class ContextTest : public TestEnvironmentCore {
public:

	void checkedDeleteObjects(std::vector<SEditorObject> const& objects) {
		auto numObjects = project.instances().size();
		for (auto obj : objects) {
			EXPECT_TRUE(std::find(project.instances().begin(), project.instances().end(), obj) != project.instances().end());
		}

		auto numDeleted = context.deleteObjects(objects);

		EXPECT_EQ(project.instances().size(), numObjects - numDeleted);
		for (auto obj : objects) {
			EXPECT_TRUE(std::find(project.instances().begin(), project.instances().end(), obj) == project.instances().end());
		}
	}
};

TEST_F(ContextTest, Simple) {
	std::shared_ptr<Foo> foo{new Foo()};
	ValueHandle o(foo);

	ValueHandle vh_x = o.get("x");
	EXPECT_EQ(vh_x.asDouble(), 2.5);
	context.set(vh_x, 3.0);
	EXPECT_EQ(vh_x.asDouble(), 3.0);

	ValueHandle vh_b = o.get("flag");
	EXPECT_EQ(vh_b.asBool(), false);
	context.set(vh_b, true);
	EXPECT_EQ(vh_b.asBool(), true);

	ValueHandle vh_i = o.get("i");
	EXPECT_EQ(vh_i.asInt(), 3);
	context.set(vh_i, 42);
	EXPECT_EQ(vh_i.asInt(), 42);

	ValueHandle vh_s = o.get("s");
	EXPECT_EQ(vh_s.asString(), "cat");
	context.set(vh_s, std::string("dog"));
	EXPECT_EQ(vh_s.asString(), "dog");

	auto changedValues = recorder.getChangedValues();
	std::map<std::string, std::set<ValueHandle>> refChangedValues{{foo->objectID(), {vh_x, vh_b, vh_i, vh_s}}};
	EXPECT_EQ(changedValues, refChangedValues);

	ValueHandle vh_vec = o.get("vec");
	EXPECT_EQ(vh_vec.get("y").asDouble(), 2.0);
}

void print_recursive(ValueHandle& handle, unsigned level = 0) {
	for (int i = 0; i < handle.size(); i++) {
		ValueHandle prop = handle[i];
		std::cout << std::string(4 * level, ' ') << prop.getPropName() << " " << getTypeName(prop.type()) << "\n";
		print_recursive(prop, level + 1);
	}
}

TEST_F(ContextTest, Complex) {
	std::shared_ptr<MockLuaScript> script{new MockLuaScript("foo")};

	EXPECT_EQ(script->size(), 5);

	auto val = &script->luaInputs_->get("in_array_struct")->asTable()[1]->asTable().get("bar")->asVec3f().y;
	EXPECT_EQ(**val, 0);

	ValueHandle s(script);

	print_recursive(s, 0);
}

TEST_F(ContextTest, Prefab) {
	std::shared_ptr<Prefab> prefab{new Prefab("prefab")};
	auto inst = std::make_shared<PrefabInstance>("inst");

	ValueHandle prefabhandle{prefab};
	ValueHandle insthandle{inst};

	ValueHandle inst_template{insthandle.get("template")};

	EXPECT_EQ(inst_template.asRef(), nullptr);
	EXPECT_EQ(prefab->instances_.size(), 0);

	context.set(inst_template, prefabhandle.rootObject());
	EXPECT_EQ(inst_template.asRef(), prefab);
	EXPECT_EQ(prefab->instances_.size(), 1);
	EXPECT_TRUE(prefab->instances_.find(inst) != prefab->instances_.end());

	context.set(inst_template, SEditorObject());
	EXPECT_EQ(inst_template.asRef(), nullptr);
	EXPECT_EQ(prefab->instances_.size(), 0);
}

TEST_F(ContextTest, Scenegraph) {
	SNode foo{new Node("foo")};
	SNode bar{new Node("bar")};
	SNode child{new Node("child")};
	SNode child2{new Node("child 2")};
	SNode child3{new Node("child 3")};

	ValueHandle foo_handle{foo};
	ValueHandle bar_handle{bar};
	ValueHandle child_handle{child};
	ValueHandle child2_handle{child2};
	ValueHandle child3_handle{child3};

	EXPECT_EQ(child->getParent(), nullptr);
	EXPECT_EQ(child2->getParent(), nullptr);
	EXPECT_EQ(child3->getParent(), nullptr);
	EXPECT_EQ(foo->children_->size(), 0);
	EXPECT_EQ(bar->children_->size(), 0);

	// Move child from scenegraph root -> foo
	context.moveScenegraphChild(child, foo);
	EXPECT_EQ(child->getParent(), foo);
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));
	EXPECT_EQ(bar->children_->size(), 0);

	// Insert child2 before child
	context.moveScenegraphChild(child2, foo, 0);
	EXPECT_EQ(child->getParent(), foo);
	EXPECT_EQ(child2->getParent(), foo);
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child2, child}));

	// Remove first child
	context.moveScenegraphChild(child2, nullptr);
	EXPECT_EQ(child->getParent(), foo);
	EXPECT_EQ(child2->getParent(), nullptr);
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));

	// Move child from foo -> bar
	context.moveScenegraphChild(child, bar);
	EXPECT_EQ(child->getParent(), bar);
	EXPECT_EQ(foo->children_->size(), 0);
	EXPECT_EQ(bar->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));

	// Move child from bar -> scenegraph root
	context.moveScenegraphChild(child, nullptr);
	EXPECT_EQ(child->getParent(), nullptr);
	EXPECT_EQ(foo->children_->size(), 0);
	EXPECT_EQ(bar->children_->size(), 0);

	// Check correct behaviour for moving towards the back
	context.moveScenegraphChild(child, foo);
	context.moveScenegraphChild(child2, foo);
	context.moveScenegraphChild(child3, foo);
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child, child2, child3}));

	context.moveScenegraphChild(child, foo, 2);
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child2, child, child3}));

	// Check for NOP handling
	recorder.reset();
	context.moveScenegraphChild(child, foo, 1);
	EXPECT_EQ(recorder.getChangedValues().size(), 0);

	context.moveScenegraphChild(child, foo, 2);
	EXPECT_EQ(recorder.getChangedValues().size(), 0);

	// iterator testing
	std::vector<SEditorObject> foo_ref_children;
	for (auto child : *foo) {
		foo_ref_children.push_back(child);
	}
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), foo_ref_children);

	foo_ref_children.clear();
	std::copy(foo->begin(), foo->end(), std::back_inserter(foo_ref_children));
	EXPECT_EQ(foo->children_->asVector<SEditorObject>(), foo_ref_children);

	foo_ref_children.clear();
	std::copy(TreeIteratorAdaptor(foo).begin(), TreeIteratorAdaptor(foo).end(), std::back_inserter(foo_ref_children));
	EXPECT_EQ(foo_ref_children, std::vector<SEditorObject>({foo, child2, child, child3}));

	foo_ref_children.clear();
	for (auto child : TreeIteratorAdaptor(foo)) {
		foo_ref_children.push_back(child);
	}
}
TEST_F(ContextTest, Mesh) {
	SMesh mesh{new Mesh("mesh")};
	ValueHandle m{mesh};
	ValueHandle m_uri{m.get("uri")};

	// Simple test of onAfterValueChanged handler:

	auto duckPath = cwd_path().append("meshes/Duck.glb").generic_string();
	context.set(m_uri, duckPath);
	EXPECT_EQ(m_uri.asString(), duckPath);
	EXPECT_EQ(mesh->materialNames(), std::vector<std::string>({"material"}));
}

TEST_F(ContextTest, MeshDeletion) {
	auto mesh = context.createObject(Mesh::typeDescription.typeName);
	EXPECT_EQ(project.instances().size(), 1);

	context.deleteObjects({mesh});
	EXPECT_EQ(project.instances().size(), 0);
}

TEST_F(ContextTest, MeshNode) {
	SMesh mesh{new Mesh("mesh")};
	SMeshNode meshnode{new MeshNode("meshnode")};
	ValueHandle m{mesh};
	ValueHandle m_uri{m.get("uri")};

	context.set(ValueHandle{meshnode, {"mesh"}}, m.rootObject());

	auto duckPath = cwd_path().append("meshes/Duck.glb").generic_string();
	context.set(ValueHandle{mesh, {"uri"}}, duckPath);
	EXPECT_EQ(m_uri.asString(), duckPath);
	EXPECT_EQ(mesh->materialNames(), std::vector<std::string>({ "material" }));
	EXPECT_EQ(meshnode->materials_->size(), 1);
	EXPECT_EQ(meshnode->materials_->name(0), "material");
	
	context.set(ValueHandle{ meshnode, {"mesh" } }, SEditorObject());
	EXPECT_EQ(meshnode->materials_->size(), 0);
}

TEST_F(ContextTest, SpecializedReferenceProperties) {
	SMesh mesh{new Mesh("mesh")};
	SMeshNode meshnode{new MeshNode("meshnode")};

	Property<SMeshNode> vmn = meshnode;
	EXPECT_EQ(*vmn, meshnode);
	// This does (intentionally) not compile:
	// vmn = mesh;
	// SMesh mymesh = *vmn;

	SNode node{new Node("node")};
	Property<SNode> vnode{{}};
	vnode = node;
	vnode = meshnode;

	EXPECT_THROW(context.set(ValueHandle{meshnode, {"mesh"}}, meshnode), std::runtime_error);
	context.set(ValueHandle{meshnode, {"mesh"}}, mesh);
}


TEST_F(ContextTest, ObjectCreation) {
	auto root = context.createObject("Node", "rootnode");
	auto mnb = context.createObject("MeshNode", "duck_node");
	auto mnl = context.createObject("MeshNode", "label_node");

	EXPECT_EQ(project.instances(), std::vector<SEditorObject>({root, mnb, mnl}));

	EXPECT_EQ(recorder.getCreatedObjects(), std::set<SEditorObject>({root, mnb, mnl}));
}

TEST_F(ContextTest, DeleteIsolatedNode) {
	auto node = context.createObject(Node::typeDescription.typeName, "n");

	checkedDeleteObjects({node});
}

TEST_F(ContextTest, DeleteNodeWithChild) {
	auto node = context.createObject(Node::typeDescription.typeName, "parent");
	auto child = context.createObject(Node::typeDescription.typeName, "child");

	context.moveScenegraphChild(child, node);
	EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));

	checkedDeleteObjects({node});
}

TEST_F(ContextTest, DeleteNodeAndChild) {
	auto node = context.createObject(Node::typeDescription.typeName, "parent");
	auto child = context.createObject(Node::typeDescription.typeName, "child");

	context.moveScenegraphChild(child, node);
	EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));

	checkedDeleteObjects({node, child});
}

TEST_F(ContextTest, DeleteNodeInParent) {
	auto node = context.createObject(Node::typeDescription.typeName, "parent");
	auto child = context.createObject(Node::typeDescription.typeName, "child");

	context.moveScenegraphChild(child, node);
	EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));

	checkedDeleteObjects({child});
	EXPECT_EQ(node->children_->size(), 0);
}

TEST_F(ContextTest, DeleteMultiNodeInParent) {
	auto node = context.createObject(Node::typeDescription.typeName, "parent");

	auto child1 = context.createObject(Node::typeDescription.typeName, "child1");
	context.moveScenegraphChild(child1, node);

	auto child2 = context.createObject(Node::typeDescription.typeName, "child2");
	context.moveScenegraphChild(child2, node);

	auto child3 = context.createObject(Node::typeDescription.typeName, "child3");
	context.moveScenegraphChild(child3, node);

	auto child4 = context.createObject(Node::typeDescription.typeName, "child4");
	context.moveScenegraphChild(child4, node);

	auto child5 = context.createObject(Node::typeDescription.typeName, "child5");
	context.moveScenegraphChild(child5, node);
	
	EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child1, child2, child3, child4, child5}));

	checkedDeleteObjects({child1, child3, child5});
	EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child2, child4}));
}

TEST_F(ContextTest, DeleteMultiNodeWithParent) {
	auto node = context.createObject(Node::typeDescription.typeName, "parent");

	auto child1 = context.createObject(Node::typeDescription.typeName, "child1");
	context.moveScenegraphChild(child1, node);

	auto child2 = context.createObject(Node::typeDescription.typeName, "child2");
	context.moveScenegraphChild(child2, node);

	auto child3 = context.createObject(Node::typeDescription.typeName, "child3");
	context.moveScenegraphChild(child3, node);

	auto child4 = context.createObject(Node::typeDescription.typeName, "child4");
	context.moveScenegraphChild(child4, node);

	auto child5 = context.createObject(Node::typeDescription.typeName, "child5");
	context.moveScenegraphChild(child5, node);

	EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child1, child2, child3, child4, child5}));

	checkedDeleteObjects({child1, child3, node, child5});
}


TEST_F(ContextTest, DeletePrefab) {
	auto prefab = std::dynamic_pointer_cast<Prefab>(context.createObject(Prefab::typeDescription.typeName, "prefab"));
	auto inst = std::dynamic_pointer_cast<PrefabInstance>(context.createObject(PrefabInstance::typeDescription.typeName, "instance"));

	EXPECT_EQ(prefab->instances_.size(), 0);

	context.set(ValueHandle(inst, {"template"}), prefab);
	EXPECT_EQ(*inst->template_, prefab);
	EXPECT_EQ(prefab->instances_.size(), 1);
	EXPECT_EQ(prefab->instances_.begin()->lock(), inst);

	checkedDeleteObjects({prefab});
	EXPECT_EQ(*inst->template_, nullptr);
}

TEST_F(ContextTest, DeletePrefabInstance) {
	auto prefab = std::dynamic_pointer_cast<Prefab>(context.createObject(Prefab::typeDescription.typeName, "prefab"));
	auto inst = std::dynamic_pointer_cast<PrefabInstance>(context.createObject(PrefabInstance::typeDescription.typeName, "instance"));

	EXPECT_EQ(prefab->instances_.size(), 0);

	context.set(ValueHandle(inst, {"template"}), prefab);
	EXPECT_EQ(*inst->template_, prefab);
	EXPECT_EQ(prefab->instances_.size(), 1);
	EXPECT_EQ(prefab->instances_.begin()->lock(), inst);

	checkedDeleteObjects({inst});
	EXPECT_EQ(prefab->instances_.size(), 0);
}

TEST_F(ContextTest, ObjectMovingOnTopLevel) {
	auto firstRoot = context.createObject(Node::typeDescription.typeName);
	auto secondRoot = context.createObject(Node::typeDescription.typeName);

	ASSERT_EQ(project.instances(), std::vector<SEditorObject>({firstRoot, secondRoot}));

	context.moveScenegraphChild({secondRoot}, {}, 0);

	ASSERT_NE(project.instances(), std::vector<SEditorObject>({secondRoot, firstRoot})) << "[!!!]  Moving top-level scenegraph nodes has been implemented / fixed. Replace ASSERT_NE macro with ASSERT_EQ macro in ContextTest unit test ObjectMovingOnTopLevel!";
}

TEST_F(ContextTest, ErrorCreationForObject) {
	auto object = context.createObject(Node::typeDescription.typeName);
	context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, object, "Some Error");
	ASSERT_TRUE(context.errors().hasError(object));
	ASSERT_EQ(
		ErrorItem(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle{object}, "Some Error"),
		context.errors().getError({ object })
	);
}

TEST_F(ContextTest, ErrorDeletionOnObjectDeletion) {
	auto object = context.createObject(Node::typeDescription.typeName);
	context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, object, "Some Error");
	context.deleteObjects({ object });
	ASSERT_FALSE(context.errors().hasError(object));
}

TEST_F(ContextTest, ErrorProjectGlobal) {
	context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle(), "Some project-global Error");
	ASSERT_EQ(context.errors().getAllErrors().size(), 1);

	context.errors().removeError(ValueHandle());
	ASSERT_TRUE(context.errors().getAllErrors().empty());
}

TEST_F(ContextTest, ValueHandleErrorDeletionOnObjectDeletion) {
	auto object = context.createObject(Node::typeDescription.typeName);
	ValueHandle handle { object, { "translation"} };
	context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, handle, "Some Error");
	context.deleteObjects({ object });
	ASSERT_FALSE(context.errors().hasError(handle));
}

TEST_F(ContextTest, copyAndPasteObjectSimple) {
	auto node = context.createObject(Node::typeDescription.typeName);
	context.pasteObjects(context.copyObjects({ node }));

	// We have 2 instances, on original and one copy
	ASSERT_EQ(2, project.instances().size());
	// They are not equal to each other
	ASSERT_NE(project.instances().at(0), project.instances().at(1));
}

TEST_F(ContextTest, copyAndPasteTwoObjectsSimple) {
	auto node = context.createObject(Node::typeDescription.typeName);
	auto node2 = context.createObject(Node::typeDescription.typeName);
	auto copyResult = context.pasteObjects(context.copyObjects({ node, node2 }));

	// node and node2 and copy of node and node2
	ASSERT_EQ(4, project.instances().size());
}

TEST_F(ContextTest, copyAndPasteParentChild) {
	auto parent = context.createObject(Node::typeDescription.typeName);
	auto child = context.createObject(Node::typeDescription.typeName);
	auto copyResult = context.pasteObjects(context.copyObjects({ parent, child }));

	// parent and child and copy of parent and child
	ASSERT_EQ(4, project.instances().size());
}

TEST_F(ContextTest, cutAndPasteObjectSimple) {
	auto node = context.createObject(Node::typeDescription.typeName);
	auto clipboardContent = context.cutObjects({ node });

	ASSERT_EQ(0, project.instances().size());

	context.pasteObjects(clipboardContent);

	ASSERT_EQ(1, project.instances().size());
}

TEST_F(ContextTest, copyAndPasteShallowSetsReferences) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName);
	auto mesh = context.createObject(Mesh::typeDescription.typeName);
	context.set({ mesh, {"uri"}}, (cwd_path() / "meshes" / "Duck.glb").string());
	context.set({ meshNode, {"mesh"}}, mesh);
	auto copyResult = context.pasteObjects(context.copyObjects({ meshNode }));
	auto meshNodeCopy = std::dynamic_pointer_cast<MeshNode>(copyResult.at(0));

	// We have 3 instances: one original, one copy, one mesh
	ASSERT_EQ(3, project.instances().size());
	ASSERT_EQ(mesh, *meshNodeCopy->mesh_);
}

TEST_F(ContextTest, copy_paste_loses_uniforms) {
	auto meshnode = create<MeshNode>("meshnode");
	auto mesh = create<Mesh>("duck_mesh");
	context.set({mesh, {"uri"}}, (cwd_path() / "meshes" / "Duck.glb").string());
	context.set({meshnode, {"mesh"}}, mesh);
	auto material = create<Material>("mat");
	context.set({material, {"uriVertex"}}, (cwd_path() / "shaders" / "basic.vert").string());
	context.set({material, {"uriFragment"}}, (cwd_path() / "shaders" / "basic.frag").string());
	context.set(ValueHandle{meshnode}.get("materials")[0].get("material"), material);
	context.set(meshnode->getMaterialPrivateHandle(0), true);

	auto uniformsHandle = ValueHandle(meshnode, {"materials"})[0].get("uniforms");
	ASSERT_TRUE(uniformsHandle);
	auto u_color = uniformsHandle.get("u_color");
	ASSERT_TRUE(u_color);

	auto clipboard = context.copyObjects({meshnode});

	context.deleteObjects({material});

	{
		auto pasted = context.pasteObjects(clipboard);
		ASSERT_EQ(pasted.size(), 1);
		auto pasted_meshnode = pasted[0]->as<MeshNode>();
		ASSERT_TRUE(pasted_meshnode != nullptr);

		auto pasted_uniformsHandle = ValueHandle(pasted_meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(pasted_uniformsHandle);
		ASSERT_FALSE(pasted_uniformsHandle.hasProperty("u_color"));
	}

	context.deleteObjects({mesh});

	{
		auto pasted = context.pasteObjects(clipboard);
		ASSERT_EQ(pasted.size(), 1);
		auto pasted_meshnode = pasted[0]->as<MeshNode>();
		ASSERT_TRUE(pasted_meshnode != nullptr);

		auto matContHandle = ValueHandle(pasted_meshnode, {"materials"});
		ASSERT_EQ(matContHandle.size(), 0);
	}
}


TEST_F(ContextTest, copyAndPasteHierarchy) {
	auto parent = context.createObject(Node::typeDescription.typeName);
	auto child = context.createObject(Node::typeDescription.typeName);
	context.moveScenegraphChild(child, parent);
	auto copyResult = context.pasteObjects(context.copyObjects({ parent }));
	auto parentCopy = std::dynamic_pointer_cast<Node>(copyResult.at(0));

	// We have 4 instances: original parent and child and copied parent and child
	ASSERT_EQ(4, project.instances().size());
}

TEST_F(ContextTest, cutAndPasteHierarchy) {
	auto parent = context.createObject(Node::typeDescription.typeName);
	auto child = context.createObject(Node::typeDescription.typeName);
	context.moveScenegraphChild(child, parent);
	auto clipboardContent = context.cutObjects({ parent });

	ASSERT_EQ(0, project.instances().size());

	context.pasteObjects(clipboardContent);

	ASSERT_EQ(2, project.instances().size());
}

TEST_F(ContextTest, copyAndPasteDeeperHierarchy) {
	auto parent = context.createObject(Node::typeDescription.typeName, "parent");
	auto child = context.createObject(Node::typeDescription.typeName, "child");
	auto sub_child = context.createObject(Node::typeDescription.typeName, "sub_child");
	context.moveScenegraphChild(child, parent);
	context.moveScenegraphChild(sub_child, child);
	auto pasteResult = context.pasteObjects(context.copyObjects({ parent }));
	
	ASSERT_EQ(1, pasteResult.size());
	auto parentCopy = std::dynamic_pointer_cast<Node>(pasteResult.at(0));

	// We have 6 instances: original parent and child and copied parent and child
	ASSERT_EQ(6, project.instances().size());
	ASSERT_EQ("parent (1)", parentCopy->objectName());
	ASSERT_EQ("child", parentCopy->children_->get(0)->asRef()->objectName());
	ASSERT_EQ("sub_child", parentCopy->children_->get(0)->asRef()->children_->get(0)->asRef()->objectName());
}

TEST_F(ContextTest, cutAndPasteDeeperHierarchy) {
	auto parent = context.createObject(Node::typeDescription.typeName, "parent");
	auto child = context.createObject(Node::typeDescription.typeName, "child");
	auto sub_child = context.createObject(Node::typeDescription.typeName, "sub_child");
	context.moveScenegraphChild(child, parent);
	context.moveScenegraphChild(sub_child, child);
	context.moveScenegraphChild(child, parent);

	auto clipboardContent = context.cutObjects({ parent });

	ASSERT_EQ(0, project.instances().size());

	auto pasteResult = context.pasteObjects(clipboardContent);

	ASSERT_EQ(3, project.instances().size());
	ASSERT_EQ(1, pasteResult.size());
	ASSERT_EQ("parent", pasteResult.at(0)->objectName());
}

TEST_F(ContextTest, pasteInvalidString) {
	auto copyResult = context.pasteObjects("SomeInvalidString");
	ASSERT_EQ(0, copyResult.size());
}

TEST_F(ContextTest, pasteInvalidJsonSchema) {
	auto copyResult = context.pasteObjects("{ \"meow\": \"wuff\" }");
	ASSERT_EQ(0, copyResult.size());
}

TEST_F(ContextTest, copyAndPasteKeepAbsolutePath) {
	auto absoluteDuckPath{(cwd_path() / "testData" / "Duck.glb").generic_string()};
	const auto sMesh{context.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{absoluteDuckPath};
	context.set({sMesh, {"uri"}}, uri);

  	auto clipboardContent = context.copyObjects({ sMesh });
	context.project()->setCurrentPath("C:/DifferentProject.file");
	auto pasteResult = context.pasteObjects(clipboardContent);
	
	ASSERT_EQ(pasteResult.front()->get("uri")->asString(), absoluteDuckPath);
}

TEST_F(ContextTest, copyAndPasteTurnRelativePathFromDifferentDriveToAbsolute) {
	context.project()->setCurrentPath((cwd_path() / "proj.file").generic_string());
	auto absoluteDuckPath{(cwd_path() / "testData" / "Duck.glb").generic_string()};
	std::string relativeDuckPath{"testData/Duck.glb"};
	const auto sMesh{context.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	context.set({sMesh, {"uri"}}, relativeDuckPath);

	auto clipboardContent = context.copyObjects({sMesh});
	context.project()->setCurrentPath("Z:/FantasyProject/OnDifferentDrive.file");
	auto pasteResult = context.pasteObjects(clipboardContent);

	ASSERT_EQ(pasteResult.front()->get("uri")->asString(), absoluteDuckPath);
}

TEST_F(ContextTest, copyAndPasteRerootRelativePathHierarchyDown) {
        context.project()->setCurrentPath((cwd_path() / "proj.file").string());
	auto relativeDuckPath{(cwd_path_relative() / "testData" / "Duck.glb").string()};
	const auto sMesh{context.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{relativeDuckPath};
	context.set({sMesh, {"uri"}}, uri);

	auto clipboardContent = context.copyObjects({ sMesh });
	context.project()->setCurrentPath((cwd_path() / "newProject" / "proj.file").string());
	auto pasteResult = context.pasteObjects(clipboardContent);
	auto newRelativeDuckPath{std::filesystem::path("..") / std::filesystem::path(relativeDuckPath)};

	ASSERT_EQ(pasteResult.front()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(ContextTest, copyAndPasteRerootRelativePathHierarchyUp) {
        context.project()->setCurrentPath((cwd_path() / "newProject"  / "proj.file").string());
	auto relativeDuckPath{(cwd_path_relative() / "testData" / "Duck.glb").generic_string()};
	const auto sMesh{context.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{relativeDuckPath};
	context.set({sMesh, {"uri"}}, uri);

	auto clipboardContent = context.copyObjects({ sMesh });
	context.project()->setCurrentPath((cwd_path() / "proj.file").string());
	auto pasteResult = context.pasteObjects(clipboardContent);
	auto newRelativeDuckPath{"newProject" / std::filesystem::path(relativeDuckPath)};

	ASSERT_EQ(pasteResult.front()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(ContextTest, cutAndPasteDifferentTypesOnTarget) {
	auto node = context.createObject(Node::typeDescription.typeName, "node");
	auto target = context.createObject(Node::typeDescription.typeName, "target");
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "lua_script");
	auto mesh = create<Mesh>("mesh");

	auto clipboardContent = context.cutObjects({ node, luaScript, mesh });

	// target is still in the project
	ASSERT_EQ(1, project.instances().size());

	auto pasteResult = context.pasteObjects(clipboardContent, target);

	ASSERT_EQ(4, project.instances().size());
	ASSERT_EQ(3, pasteResult.size());
	ASSERT_TRUE(Queries::findByName(pasteResult, "node") != nullptr);
	ASSERT_TRUE(Queries::findByName(pasteResult, "lua_script") != nullptr);
	ASSERT_TRUE(Queries::findByName(pasteResult, "mesh") != nullptr);
	ASSERT_EQ(target, Queries::findByName(pasteResult, "node")->getParent());
	ASSERT_EQ(target, Queries::findByName(pasteResult, "lua_script")->getParent());
	ASSERT_EQ(nullptr, Queries::findByName(pasteResult, "mesh")->getParent());
}

TEST_F(ContextTest, deepCut) {
	auto node = context.createObject(Node::typeDescription.typeName, "node");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "meshNode");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "mesh");
	context.moveScenegraphChild(meshNode, node);
	context.set({ meshNode, { "mesh" }}, mesh);

	auto serial = context.cutObjects({node}, true);
	ASSERT_EQ(0, context.project()->instances().size());
}

TEST_F(ContextTest, shallowCopyLink) {
	auto objs { raco::createLinkedScene(*this) };
	ASSERT_EQ(1, context.project()->links().size());

	context.pasteObjects(context.copyObjects({ std::get<1>(objs) }));

	ASSERT_EQ(2, context.project()->links().size());
}

TEST_F(ContextTest, shallowCutLink) {
	auto objs { raco::createLinkedScene(*this) };
	ASSERT_EQ(1, context.project()->links().size());

	auto clipboard = context.cutObjects({ std::get<1>(objs) });
	ASSERT_EQ(0, context.project()->links().size());

	context.pasteObjects(clipboard);

	ASSERT_EQ(1, context.project()->links().size());
}

TEST_F(ContextTest, shallowCopyLink_deletedStartObject) {
	auto objs { raco::createLinkedScene(*this) };
	ASSERT_EQ(1, context.project()->links().size());

	auto clipboard = context.copyObjects({ std::get<1>(objs) });
	context.removeLink( std::get<2>(objs)->endProp() );
	context.deleteObjects({std::get<2>(objs)->startProp().object() });
	ASSERT_EQ(0, context.project()->links().size());

	// No link if start is no longer valid
	context.pasteObjects(clipboard);
	ASSERT_EQ(0, context.project()->links().size());
}

TEST_F(ContextTest, copyAndPasteTwoNodesUniqueName) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	auto copiedObjs = context.copyObjects({node});
	auto node2 = context.pasteObjects(copiedObjs).front();

	ASSERT_EQ(node2->objectName(), "Node (1)");
}

TEST_F(ContextTest, copyAndPasteAsChildrenNodesUniqueName) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	auto copiedObjs = context.copyObjects({node});
	auto node2 = context.pasteObjects(copiedObjs, node).front();
	auto node3 = context.pasteObjects(copiedObjs, node2).front();
	auto node4 = context.pasteObjects(copiedObjs, node2).front();

	ASSERT_EQ(node->objectName(), "Node");
	ASSERT_EQ(node2->objectName(), "Node");
	ASSERT_EQ(node3->objectName(), "Node");
	ASSERT_EQ(node4->objectName(), "Node (1)");
}

TEST_F(ContextTest, cutAndPasteAsChildrenNodesUniqueName) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	auto cutNode = context.createObject(Node::typeDescription.typeName, "CutMe");
	auto cutObjs = context.cutObjects({cutNode});
	auto node2 = context.pasteObjects(cutObjs, node).front();
	auto node3 = context.pasteObjects(cutObjs, node2).front();
	auto node4 = context.pasteObjects(cutObjs, node2).front();

	ASSERT_EQ(node->objectName(), "Node");
	ASSERT_EQ(node2->objectName(), "CutMe");
	ASSERT_EQ(node3->objectName(), "CutMe");
	ASSERT_EQ(node4->objectName(), "CutMe (1)");
}

TEST_F(ContextTest, copyAndPasteStructureUniqueName) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	auto child1 = context.createObject(Node::typeDescription.typeName, "Child1");
	auto child11 = context.createObject(Node::typeDescription.typeName, "Child1_1");
	auto child2 = context.createObject(Node::typeDescription.typeName, "Child2");
	context.moveScenegraphChild(child1, node);
	context.moveScenegraphChild(child2, node);
	context.moveScenegraphChild(child11, child1);

	auto copiedObjs = context.copyObjects({node});

	auto pastedObjs = context.pasteObjects(copiedObjs);

	ASSERT_EQ(pastedObjs[0]->objectName(), "Node (1)");
	ASSERT_EQ(pastedObjs.front()->children_->get(0)->asRef()->objectName(), "Child1");
	ASSERT_EQ(pastedObjs.front()->children_->get(1)->asRef()->objectName(), "Child2");
	ASSERT_EQ(pastedObjs.front()->children_->get(0)->asRef()->children_->get(0)->asRef()->objectName(), "Child1_1");
}

TEST_F(ContextTest, cutAndPasteStructureUniqueName) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	auto child1 = context.createObject(Node::typeDescription.typeName, "Child1");
	auto child11 = context.createObject(Node::typeDescription.typeName, "Child1_1");
	auto child2 = context.createObject(Node::typeDescription.typeName, "Child2");
	context.moveScenegraphChild(child1, node);
	context.moveScenegraphChild(child2, node);
	context.moveScenegraphChild(child11, child1);

	auto cutObjs = context.cutObjects({node});

	auto pastedObjs = context.pasteObjects(cutObjs);

	ASSERT_EQ(pastedObjs[0]->objectName(), "Node");
	ASSERT_EQ(pastedObjs.front()->children_->get(0)->asRef()->objectName(), "Child1");
	ASSERT_EQ(pastedObjs.front()->children_->get(1)->asRef()->objectName(), "Child2");
	ASSERT_EQ(pastedObjs.front()->children_->get(0)->asRef()->children_->get(0)->asRef()->objectName(), "Child1_1");
}

TEST_F(ContextTest, cutAndPasteNodeUniqueName) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	auto cutObjs = context.cutObjects({node});
	auto node2 = context.pasteObjects(cutObjs).front();

	ASSERT_EQ(node2->objectName(), "Node");
}

TEST_F(ContextTest, queryLinkConnectedToObjectsReturnsNoDuplicateLinks) {
	auto objs{raco::createLinkedScene(*this)};

	auto totalLinks = raco::core::Queries::getLinksConnectedToObjects(
		*context.project(), {context.project()->instances().begin(), context.project()->instances().end()}, true, true);

	ASSERT_EQ(totalLinks.size(), 1);
}

TEST_F(ContextTest, meshnode_assign_mat_with_private_material) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshnode = create_meshnode("meshnode", mesh, nullptr);
	context.set(meshnode->getMaterialPrivateHandle(0), true);
	context.set(meshnode->getMaterialHandle(0), material);

	ASSERT_FALSE(meshnode->materialPrivate(1));
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 1);

	context.set(meshnode->getMaterialPrivateHandle(0), false);
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);

	context.set(meshnode->getMaterialPrivateHandle(0), true);
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 1);
}

TEST_F(ContextTest, meshnode_assign_mat_with_shared_material) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	// default is shared material, don't need to set flag

	ASSERT_FALSE(meshnode->materialPrivate(0));
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);

	context.set(meshnode->getMaterialPrivateHandle(0), true);
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 1);

	context.set(meshnode->getMaterialPrivateHandle(0), false);
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);
}

TEST_F(ContextTest, meshnode_shared_mat_modify_material) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	// default is shared material, don't need to set flag

	ASSERT_FALSE(meshnode->materialPrivate(0));
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);

	context.set({material, {"uniforms", "u_color", "x"}}, 0.5);
	ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);
}



