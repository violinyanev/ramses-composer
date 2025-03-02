/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Queries.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"
#include "application/RaCoProject.h"
#include "application/RaCoApplication.h"
#include "components/RaCoPreferences.h"
#include "core/PathManager.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "utils/PathUtils.h"

class RaCoProjectFixture : public RacoBaseTest<> {
public:
	raco::ramses_base::HeadlessEngineBackend backend{};
};

using raco::application::RaCoApplication;

TEST_F(RaCoProjectFixture, saveLoadWithLink) {
	{
		RaCoApplication app{backend};
		raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
end
function run()
end
	)");
		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE(app.activeRaCoProject().project()->links()[0]->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenAndValidLink) {
	{
		RaCoApplication app{backend};
		const auto luaScript{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
		const auto node{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.translation = VEC3F
	OUT.rotation = VEC3F
end
function run()
end
	)");
		app.activeRaCoProject().commandInterface()->set({luaScript, {"uri"}}, (cwd_path() / "lua_script.lua").string());
		app.activeRaCoProject().commandInterface()->addLink({luaScript, {"luaOutputs", "translation"}}, {node, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({luaScript, {"luaOutputs", "rotation"}}, {node, {"rotation"}});

		ASSERT_EQ(2, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[0]->isValid());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[1]->isValid());

		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
	OUT.rotation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(2, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE(app.activeRaCoProject().project()->links()[0]->isValid());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[1]->isValid());
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.translation = VEC3F
	OUT.rotation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(2, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[0]->isValid());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[1]->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveWithValidLinkLoadWithBrokenLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE(app.activeRaCoProject().project()->links()[0]->isValid());
	}
}


TEST_F(RaCoProjectFixture, saveWithBrokenLinkLoadWithValidLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
end
function run()
end
	)");
		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.translation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[0]->isValid());
	}
}


TEST_F(RaCoProjectFixture, saveLoadWithLinkRemoveOutputPropertyBeforeLoading) {
	{
		RaCoApplication app{backend};
		raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}

	// replace OUT.translation with OUT.scale in external script file
	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.scale = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		app.doOneLoop();
		ASSERT_EQ(app.activeRaCoProject().project()->links().size(), 1);
		ASSERT_EQ(app.activeRaCoProject().project()->links()[0]->isValid(), false);
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithLuaScriptNewOutputPropertyGetsCalculated) {
	auto luaScriptPath = (cwd_path() / "lua_script.lua").string();

	raco::utils::file::write(luaScriptPath, R"(
function interface()
	IN.integer = INT
	OUT.integer = INT
end


function run()
	OUT.integer = IN.integer
end

	)");

	{
		RaCoApplication app{backend};
		const auto luaScript{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
		app.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"uri"}}, luaScriptPath);
		app.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"luaInputs", "integer"}}, 5);
		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	IN.integer = INT
	OUT.integerTwo = INT
end


function run()
	OUT.integerTwo = IN.integer
end

	)");
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		app.doOneLoop();
		auto luaScript = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");
		auto newPropertyOutput = raco::core::ValueHandle{luaScript, {"luaOutputs", "integerTwo"}}.asInt();
		ASSERT_EQ(newPropertyOutput, 5);
	}
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyDown) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id");

	std::string relativeUri{"Duck.glb"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str());
	std::string newRelativeDuckPath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyUp) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project" / "project.file").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id");

	std::string relativeUri{"Duck.glb"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	app.activeRaCoProject().saveAs((cwd_path() / "project.file").string().c_str());
	std::string newRelativeDuckPath{"project/" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(RaCoProjectFixture, saveAsMaterialRerootRelativeURI) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Material::typeDescription.typeName, "material");

	std::string relativeUri{"relativeURI"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uriVertex"}}, relativeUri);
	app.activeRaCoProject().commandInterface()->set({mesh, {"uriFragment"}}, relativeUri);

	app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str());
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uriVertex")->asString(), newRelativePath);
	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uriFragment")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsLuaScriptRerootRelativeURI) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str());
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsSimulateSavingFromNewProjectCorrectlyRerootedRelativeURI) {
	RaCoApplication app{backend};
	std::filesystem::create_directory(cwd_path() / "project");
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	app.activeRaCoProject().saveAs(QString::fromStdString((cwd_path() / "project" / "project.file").string()));

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), relativeUri);
}

TEST_F(RaCoProjectFixture, saveAsToDifferentDriveSetsRelativeURIsToAbsolute) {
	RaCoApplication app{backend};
	std::filesystem::create_directory(cwd_path() / "project");
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	app.activeRaCoProject().saveAs("Z:/projectOnDifferentDrive.rca");

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), (cwd_path() / "project" / relativeUri).generic_string());
}

TEST_F(RaCoProjectFixture, idChange) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	app.doOneLoop();
	ASSERT_EQ(123u, app.sceneBackend()->currentSceneIdValue());

	app.activeRaCoProject().commandInterface()->set({app.activeRaCoProject().project()->settings(), {"sceneId"}}, 1024);
	app.doOneLoop();
	ASSERT_EQ(1024u, app.sceneBackend()->currentSceneIdValue());
}

TEST_F(RaCoProjectFixture, enableTimerFlagChange) {
	RaCoApplication app{backend};
	const auto PROJECT_PATH = QString::fromStdString((cwd_path() / "project.file").string());
	app.activeRaCoProject().commandInterface()->set({app.activeRaCoProject().project()->settings(), {"enableTimerFlag"}}, true);
	app.activeRaCoProject().saveAs(PROJECT_PATH);
	app.switchActiveRaCoProject(PROJECT_PATH);
	ASSERT_EQ(app.activeRaCoProject().project()->settings()->enableTimerFlag_.asBool(), true);
}

TEST_F(RaCoProjectFixture, restoredLinkWorksInLogicEngine) {
	RaCoApplication app{backend};
	auto start{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start", "start")};
	app.activeRaCoProject().commandInterface()->set({start, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	app.doOneLoop();

	auto end{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end", "end")};
	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->addLink({start, {"luaOutputs", "out_float"}}, {end, {"luaInputs", "in_float"}});
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, std::string());
	app.doOneLoop();
	ASSERT_EQ(app.activeRaCoProject() .project()->links().size(), 1);
	ASSERT_FALSE(app.activeRaCoProject() .project()->links()[0]->isValid());

	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	app.activeRaCoProject().commandInterface()->set({start, {"luaInputs", "in_float"}}, 3.0);
	app.doOneLoop();
	ASSERT_EQ(raco::core::ValueHandle(end, {"luaInputs", "in_float"}).asDouble(), 3.0);
}

TEST_F(RaCoProjectFixture, brokenLinkDoesNotResetProperties) {
	RaCoApplication app{backend};
	auto linkStart{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start", "start")};
	app.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());

	auto linkEnd{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end", "end")};
	app.activeRaCoProject().commandInterface()->set({linkEnd, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->addLink({linkStart, {"luaOutputs", "ointeger"}}, {linkEnd, {"luaInputs", "integer"}});
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkStart, {"luaInputs", "integer"}}, 10);
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, std::string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkEnd, {"luaInputs", "float"}}, 20.0);
	app.doOneLoop();

	auto output_int = raco::core::ValueHandle{linkEnd, {"luaOutputs", "ointeger"}}.asInt();
	auto output_bool = raco::core::ValueHandle{linkEnd, {"luaOutputs", "flag"}}.asBool();
	ASSERT_EQ(output_int, 40);
	ASSERT_EQ(output_bool, true);
}

TEST_F(RaCoProjectFixture, launchApplicationWithNoResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	auto newProjectFolder = (cwd_path() / "newProject").generic_string();
	std::filesystem::create_directory(newProjectFolder);
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(newProjectFolder);
	RaCoApplication app{backend};

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::DEFAULT_PROJECT_SUB_DIRECTORY), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::MESH_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SCRIPT_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::IMAGE_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SHADER_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SHADER_SUB_DIRECTORY);
}

TEST_F(RaCoProjectFixture, launchApplicationWithResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(cwd_path().generic_string());
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::MESH_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::SHADER_SUB_DIRECTORY);

	RaCoApplication app{backend};
	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::DEFAULT_PROJECT_SUB_DIRECTORY), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::MESH_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SCRIPT_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::IMAGE_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SHADER_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SHADER_SUB_DIRECTORY);
}

TEST_F(RaCoProjectFixture, saveAsNewProjectGeneratesResourceSubFolders) {
	auto newProjectFolder = (cwd_path() / "newProject").generic_string();
	std::filesystem::create_directory(newProjectFolder);

	RaCoApplication app{backend};
	app.activeRaCoProject().saveAs(QString::fromStdString(newProjectFolder + "/project.rca"));

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::DEFAULT_PROJECT_SUB_DIRECTORY), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::MESH_SUB_DIRECTORY);
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + raco::core::PathManager::MESH_SUB_DIRECTORY));
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SCRIPT_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + raco::core::PathManager::SCRIPT_SUB_DIRECTORY));
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::IMAGE_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + raco::core::PathManager::IMAGE_SUB_DIRECTORY));
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SHADER_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SHADER_SUB_DIRECTORY);
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + raco::core::PathManager::SHADER_SUB_DIRECTORY));
}

TEST_F(RaCoProjectFixture, saveAsThenCreateNewProjectResetsCachedPaths) {
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(cwd_path().generic_string());
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::MESH_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / raco::core::PathManager::SHADER_SUB_DIRECTORY);
	std::filesystem::create_directory(cwd_path() / "newProject");

	RaCoApplication app{backend};
	app.activeRaCoProject().saveAs(QString::fromStdString((cwd_path() / "newProject/project.rca").generic_string()));
	app.switchActiveRaCoProject("");

	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::DEFAULT_PROJECT_SUB_DIRECTORY), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::MESH_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SCRIPT_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::IMAGE_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SHADER_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SHADER_SUB_DIRECTORY);
}

TEST_F(RaCoProjectFixture, saveAsThenLoadProjectProperlySetCachedPaths) {
	auto newProjectFolder = (cwd_path() / "newProject").generic_string();
	std::filesystem::create_directory(newProjectFolder);

	RaCoApplication app{backend};
	app.activeRaCoProject().saveAs(QString::fromStdString(newProjectFolder + "/project.rca"));
	app.switchActiveRaCoProject("");
	app.switchActiveRaCoProject(QString::fromStdString(newProjectFolder + "/project.rca"));

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::DEFAULT_PROJECT_SUB_DIRECTORY), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::MESH_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SCRIPT_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SCRIPT_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::IMAGE_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::IMAGE_SUB_DIRECTORY);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::SHADER_SUB_DIRECTORY), newProjectFolder + "/" + raco::core::PathManager::SHADER_SUB_DIRECTORY);
}
