/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "serialization/Serialization.h"
#include "serialization/SerializationKeys.h"

#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "core/ExternalReferenceAnnotation.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "utils/FileUtils.h"

#include <gtest/gtest.h>

using namespace raco::user_types;

struct DeserializationTest : public TestEnvironmentCore {
	raco::serialization::DeserializationFactory deserializationFactory() noexcept {
		return {[this](const std::string& typeName) -> raco::serialization::SReflectionInterface {
					if (typeName == raco::core::Link::typeDescription.typeName) {
						return std::make_shared<raco::core::Link>();
					} else {
						return objectFactory()->createObject(typeName);
					}
				},
			[this](const std::string& type) {
				return objectFactory()->createAnnotation(type);
			},
			[this](const std::string& valueType) -> raco::data_storage::ValueBase* {
				if (valueType == "LuaScript::DisplayNameAnnotation") {
					return new Property<raco::user_types::SLuaScript, raco::data_storage::DisplayNameAnnotation>({}, {});
				}
				return objectFactory()->createValue(valueType);
			}};
	}
};

TEST_F(DeserializationTest, deserializeNode) {
	auto result = raco::serialization::deserializeObject(raco::utils::file::read((cwd_path() / "expectations" / "Node.json").string()), deserializationFactory());

	ASSERT_EQ(raco::user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	ASSERT_EQ(sNode->objectID(), "node_id");
	ASSERT_EQ(sNode->objectName(), "node");
	ASSERT_EQ(100.0, *sNode->scale_->z.staticQuery<raco::data_storage::RangeAnnotation<double>>().max_);
}

TEST_F(DeserializationTest, deserializeNodeRotated) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "NodeRotated.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	ASSERT_EQ(*sNode->rotation_->x, 90.0);
	ASSERT_EQ(*sNode->rotation_->y, -90.0);
	ASSERT_EQ(*sNode->rotation_->z, 180.0);
}

TEST_F(DeserializationTest, deserializeNodeWithAnnotations) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "NodeWithAnnotations.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	auto anno = sNode->query<raco::core::ExternalReferenceAnnotation>();
	ASSERT_TRUE(anno != nullptr);
	ASSERT_EQ(*anno->projectID_, std::string("base_id"));
}

TEST_F(DeserializationTest, deserializeMeshNodeWithMesh) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "MeshNodeWithMesh.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::MeshNode::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SMeshNode sMeshNode{std::dynamic_pointer_cast<MeshNode>(result.object)};
	ASSERT_EQ(1, result.references.size());
	ASSERT_EQ(result.references.at(&sMeshNode->mesh_), "mesh_id");
	ASSERT_EQ("Material", sMeshNode->materials_->get("material")->asTable().get("material")->typeName());
}

TEST_F(DeserializationTest, deserializeNodeWithMeshNode) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "NodeWithChildMeshNode.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	ASSERT_EQ(1, result.references.size());
}

TEST_F(DeserializationTest, deserializeMesh) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "Mesh.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::Mesh::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SMesh sMesh{std::dynamic_pointer_cast<Mesh>(result.object)};
	ASSERT_EQ(0, result.references.size());
	ASSERT_EQ(1, sMesh->materialNames_->size());
}

TEST_F(DeserializationTest, deserializeLuaScript) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScript.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	ASSERT_EQ(0, result.references.size());
	ASSERT_EQ(0, sLuaScript->luaInputs_->size());
	ASSERT_EQ(0, sLuaScript->luaOutputs_->size());
}

TEST_F(DeserializationTest, deserializeLuaScriptInStruct) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptInStruct.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	ASSERT_EQ(0, result.references.size());
	ASSERT_EQ(1, sLuaScript->luaInputs_->size());
	ASSERT_EQ(raco::data_storage::PrimitiveType::Double, sLuaScript->luaInputs_->get(0)->asTable().get(0)->type());
	ASSERT_EQ(raco::data_storage::PrimitiveType::Double, sLuaScript->luaInputs_->get(0)->asTable().get(1)->type());
	ASSERT_EQ(0, sLuaScript->luaOutputs_->size());
}

TEST_F(DeserializationTest, deserializeLuaScriptInSpecificPropNames) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptSpecificPropNames.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
}

TEST_F(DeserializationTest, deserializeLuaScriptWithRefToUserTypeWithAnnotation) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptWithRefToUserTypeWithAnnotation.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	auto* property{sLuaScript->luaInputs_->get(sLuaScript->luaInputs_->index("ref"))};
	ASSERT_EQ("LuaScript::DisplayNameAnnotation", property->typeName());
	ASSERT_EQ(1, property->baseAnnotationPtrs().size());
	ASSERT_TRUE(property->dynamicQuery<raco::data_storage::DisplayNameAnnotation>() != nullptr);
	ASSERT_EQ("BLUBB", *property->query<raco::data_storage::DisplayNameAnnotation>()->name_);
}

TEST_F(DeserializationTest, deserializeLuaScriptWithURI) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptWithURI.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	auto* property{sLuaScript->luaInputs_->get(sLuaScript->luaInputs_->index("uri"))};
	ASSERT_EQ("String::URIAnnotation", property->typeName());
	ASSERT_EQ(1, property->baseAnnotationPtrs().size());
	ASSERT_TRUE(property->dynamicQuery<raco::data_storage::URIAnnotation>() != nullptr);
}

TEST_F(DeserializationTest, deserializeLuaScriptWithAnnotatedDouble) {
	auto result = raco::serialization::deserializeObject(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptWithAnnotatedDouble.json").string()), deserializationFactory());
	ASSERT_EQ(raco::user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};

	auto* property = sLuaScript->luaInputs_->get(sLuaScript->luaInputs_->index("double"));
	ASSERT_EQ("Double", *property->query<raco::user_types::DisplayNameAnnotation>()->name_);
	ASSERT_EQ(-10.0, *property->query<raco::user_types::RangeAnnotation<double>>()->min_);
	ASSERT_EQ(10.0, *property->query<raco::user_types::RangeAnnotation<double>>()->max_);
}

TEST_F(DeserializationTest, deserializeVersionArray) {
	using raco::serialization::ProjectDeserializationInfo;
	QJsonObject fakeProjectJSON;

	auto compareVersionValues = [this, &fakeProjectJSON](raco::serialization::DeserializedVersion&& expectedRamsesVer, raco::serialization::DeserializedVersion&& expectedLogicEngineVer, raco::serialization::DeserializedVersion&& expectedRaCoVer) {
		QJsonDocument fakeProjectJSONFile(fakeProjectJSON);
		auto deserializedProjectJSON = raco::serialization::deserializeProject(fakeProjectJSONFile, deserializationFactory());

		auto deserializedVersionsAreEqual = [](const auto& lhVersion, const auto& rhVersion) {
			return lhVersion.major == rhVersion.major && lhVersion.minor == rhVersion.minor && lhVersion.patch == rhVersion.patch;
		};

		ASSERT_TRUE(deserializedVersionsAreEqual(deserializedProjectJSON.ramsesVersion, expectedRamsesVer));
		ASSERT_TRUE(deserializedVersionsAreEqual(deserializedProjectJSON.ramsesLogicEngineVersion, expectedLogicEngineVer));
		ASSERT_TRUE(deserializedVersionsAreEqual(deserializedProjectJSON.raCoVersion, expectedRaCoVer));
	};

	compareVersionValues(
		{ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION},
		{ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION},
		{ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION});

	fakeProjectJSON[raco::serialization::keys::RAMSES_VERSION] = QJsonArray{1};
	fakeProjectJSON[raco::serialization::keys::RAMSES_LOGIC_ENGINE_VERSION] = QJsonArray{1, 2, 3};
	fakeProjectJSON[raco::serialization::keys::RAMSES_COMPOSER_VERSION] = QJsonArray{99, 1, 3, 5};

	compareVersionValues(
		{1, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION},
		{1, 2, 3},
		{99, 1, 3});
}

TEST_F(DeserializationTest, deserializeObjects_luaScriptLinkedToNode_outputsAreDeserialized) {
	auto result = raco::serialization::deserializeObjects(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptLinkedToNode.json").string()), deserializationFactory());

	raco::user_types::SLuaScript sScript{ raco::select<raco::user_types::LuaScript>(result.objects)};
	ASSERT_EQ(1, sScript->luaOutputs_->size());
}

TEST_F(DeserializationTest, deserializeObjects_luaScriptLinkedToNode) {
	auto result = raco::serialization::deserializeObjects(
		raco::utils::file::read((cwd_path() / "expectations" / "LuaScriptLinkedToNode.json").string()), deserializationFactory());

	std::vector<raco::core::SEditorObject> objects{};
	objects.reserve(result.objects.size());
	for (auto& i : result.objects) {
		objects.push_back(std::dynamic_pointer_cast<raco::core::EditorObject>(i));
	}
	for (auto& ref : result.references) {
		*ref.first = *std::find_if(objects.begin(), objects.end(), [&ref](const raco::core::SEditorObject& obj) {
			return obj->objectID() == ref.second;
		});
	}

	ASSERT_EQ(2, result.objects.size());
	ASSERT_EQ(1, result.links.size());
	ASSERT_EQ(2, result.references.size());

	auto sLink{std::dynamic_pointer_cast<raco::core::Link>(result.links.at(0))};
	raco::user_types::SLuaScript sLuaScript{raco::select<raco::user_types::LuaScript>(result.objects)};
	raco::user_types::SNode sNode{raco::select<raco::user_types::Node>(result.objects)};

	raco::core::PropertyDescriptor startProp {sLuaScript, {"luaOutputs", "translation"}};
	EXPECT_EQ(startProp, sLink->startProp());
	raco::core::PropertyDescriptor endProp{sNode, {"translation"}};
	EXPECT_EQ(endProp, sLink->endProp());

	std::set<std::string> refRootObjectIDs{"node_id", "lua_script_id"};
	EXPECT_EQ(result.rootObjectIDs, refRootObjectIDs);
}