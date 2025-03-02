/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "EditorTestFixture.h"
#include "core/EditorObject.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/editors/BoolEditor.h"
#include "property_browser/editors/DoubleEditor.h"
#include "property_browser/editors/EnumerationEditor.h"
#include "property_browser/editors/IntEditor.h"
#include "property_browser/editors/StringEditor.h"
#include "property_browser/editors/URIEditor.h"
#include "property_browser/editors/RefEditor.h"
#include "property_browser/editors/VecNTEditor.h"
#include "components/DataChangeDispatcher.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include <optional>

using raco::core::CommandInterface;
using raco::core::ValueHandle;
using raco::property_browser::PropertyBrowserItem;

struct TestParam {
	std::string testName;
	std::function<QWidget*(PropertyBrowserItem*)> createEditor;
	std::string editorObjectTypeName;
	std::vector<std::string> editorHandleNames;
	std::function<void(CommandInterface& commandInterface, const ValueHandle& handle)> setValue;
	std::vector<std::string> valueHandleNames{};
};

struct PrimitiveEditorDataChangeFixture : public EditorTestFixtureT<::testing::TestWithParam<TestParam>> {
	std::filesystem::path cwd_path() const override {
		std::string testCaseName{::testing::UnitTest::GetInstance()->current_test_info()->name()};
		testCaseName = testCaseName.substr(0, testCaseName.find("#GetParam()"));

		std::replace(testCaseName.begin(), testCaseName.end(), '/', '\\');
		auto result(std::filesystem::current_path() / testCaseName);
		return result;
	}
	struct PrintToStringParamName {
		template <class ParamType>
		std::string operator()(const testing::TestParamInfo<ParamType>& info) const {
			return static_cast<TestParam>(info.param).testName;
		}
	};
};

TEST_P(PrimitiveEditorDataChangeFixture, noValueChangeRecorded_onCreation) {
	auto node{context.createObject(GetParam().editorObjectTypeName)};
	dispatch();
	application.processEvents();

	PropertyBrowserItem item{{node, GetParam().editorHandleNames}, dataChangeDispatcher, &commandInterface, &model};
	QWidget* editor{GetParam().createEditor(&item)};

	application.processEvents();

	ASSERT_FALSE(raco::isValueChanged(recorder, {node, GetParam().editorHandleNames}));
	delete editor;
}

TEST_P(PrimitiveEditorDataChangeFixture, noValueChangeRecorded_onForeignSet) {
	auto node{context.createObject(GetParam().editorObjectTypeName)};
	PropertyBrowserItem item{{node, GetParam().editorHandleNames}, dataChangeDispatcher, &commandInterface, &model};
	QWidget* editor{GetParam().createEditor(&item)};
	dispatch();
	application.processEvents();

	if (GetParam().valueHandleNames.size() > 0)
		GetParam().setValue(commandInterface, {node, GetParam().valueHandleNames});
	else
		GetParam().setValue(commandInterface, {node, GetParam().editorHandleNames});
	dispatch();
	application.processEvents();

	ASSERT_FALSE(raco::isValueChanged(recorder, {node, GetParam().editorHandleNames}));
	if (GetParam().valueHandleNames.size() > 0)
		ASSERT_FALSE(raco::isValueChanged(recorder, {node, GetParam().valueHandleNames}));
	delete editor;
}

using raco::user_types::Material;
using raco::user_types::MeshNode;
using raco::user_types::Mesh;
using raco::user_types::Node;
INSTANTIATE_TEST_SUITE_P(
	PrimitiveEditorsDataChangeTest,
	PrimitiveEditorDataChangeFixture,
	::testing::Values(
		TestParam{
			"BoolEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::BoolEditor(item, nullptr); },
			Node::typeDescription.typeName,
			{"visible"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, false); }},
		TestParam{
			"DoubleEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::DoubleEditor(item, nullptr); },
			Node::typeDescription.typeName,
			{"translation", "x"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, 5.0); }},
		TestParam{
			"EnumerationEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::EnumerationEditor(item, nullptr); },
			Material::typeDescription.typeName,
			{"options", "blendOperationColor"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, 4); }},
		TestParam{
			"IntEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::IntEditor(item, nullptr); },
			MeshNode::typeDescription.typeName,
			{"instanceCount"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, 4); }},
		TestParam{
			"RefEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::RefEditor(item, nullptr); },
			MeshNode::typeDescription.typeName,
			{"mesh"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, commandInterface.createObject(Mesh::typeDescription.typeName)); }},
		TestParam{
			"StringEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::StringEditor(item, nullptr); },
			Node::typeDescription.typeName,
			{"objectName"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, std::string{"New Object Name"}); }},
		TestParam{
			"URIEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::URIEditor(item, nullptr); },
			Material::typeDescription.typeName,
			{"uriVertex"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, std::string{"some_uri"}); }},
		TestParam{
			"VecNTEditor",
			[](PropertyBrowserItem* item) -> QWidget* { return new raco::property_browser::Vec3fEditor(item, nullptr); },
			Node::typeDescription.typeName,
			{"translation"},
			[](CommandInterface& commandInterface, const ValueHandle& handle) { commandInterface.set(handle, 5.0); },
			{"translation", "x"},
		}),
	PrimitiveEditorDataChangeFixture::PrintToStringParamName());
