/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_base/Utils.h"

#include "ramses_base/LogicEngine.h"
#include <ramses-logic/Logger.h>
#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/Property.h>
#include <sstream>

namespace raco::ramses_base {

std::map<std::string, ramses::EEffectUniformSemantic> defaultUniformSemantics = {
	// Convention take over from Sicht/Absicht
	{"u_MMatrix", ramses::EEffectUniformSemantic::ModelMatrix},
	{"u_MVMatrix", ramses::EEffectUniformSemantic::ModelViewMatrix},
	{"u_MVPMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix},
	{"u_PMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix},
	{"u_VMatrix", ramses::EEffectUniformSemantic::ViewMatrix},
	{"u_NMatrix", ramses::EEffectUniformSemantic::NormalMatrix},
	{"u_CameraWorldPosition", ramses::EEffectUniformSemantic::CameraWorldPosition},
	{"u_resolution", ramses::EEffectUniformSemantic::DisplayBufferResolution},

	// Camel case attribute names
	{"uWorldMatrix", ramses::EEffectUniformSemantic::ModelMatrix},
	{"uWorldViewMatrix", ramses::EEffectUniformSemantic::ModelViewMatrix},
	{"uWorldViewProjectionMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix},
	{"uProjectionMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix},
	{"uViewMatrix", ramses::EEffectUniformSemantic::ViewMatrix},
	{"uNormalMatrix", ramses::EEffectUniformSemantic::NormalMatrix},
	{"uCameraPosition", ramses::EEffectUniformSemantic::CameraWorldPosition},
	{"uResolution", ramses::EEffectUniformSemantic::DisplayBufferResolution}};

static std::map<ramses::EEffectInputDataType, raco::core::EnginePrimitive> shaderTypeMap = {
	{ramses::EEffectInputDataType_Int32, raco::core::EnginePrimitive::Int32},
	{ramses::EEffectInputDataType_UInt16, raco::core::EnginePrimitive::UInt16},
	{ramses::EEffectInputDataType_UInt32, raco::core::EnginePrimitive::UInt32},
	{ramses::EEffectInputDataType_Float, raco::core::EnginePrimitive::Double},

	{ramses::EEffectInputDataType_Vector2F, raco::core::EnginePrimitive::Vec2f},
	{ramses::EEffectInputDataType_Vector3F, raco::core::EnginePrimitive::Vec3f},
	{ramses::EEffectInputDataType_Vector4F, raco::core::EnginePrimitive::Vec4f},

	{ramses::EEffectInputDataType_Vector2I, raco::core::EnginePrimitive::Vec2i},
	{ramses::EEffectInputDataType_Vector3I, raco::core::EnginePrimitive::Vec3i},
	{ramses::EEffectInputDataType_Vector4I, raco::core::EnginePrimitive::Vec4i},

	//{ramses::EEffectInputDataType_Matrix22F, },
	//{ramses::EEffectInputDataType_Matrix33F, },
	//{ramses::EEffectInputDataType_Matrix44F, },

	{ramses::EEffectInputDataType_TextureSampler2D, raco::core::EnginePrimitive::TextureSampler2D},
	{ramses::EEffectInputDataType_TextureSampler3D, raco::core::EnginePrimitive::TextureSampler3D},
	{ramses::EEffectInputDataType_TextureSamplerCube, raco::core::EnginePrimitive::TextureSamplerCube}};

std::unique_ptr<ramses::EffectDescription> createEffectDescription(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, const std::string &shaderDefines) {
	std::unique_ptr<ramses::EffectDescription> description{new ramses::EffectDescription};

	if (!shaderDefines.empty()) {
		std::istringstream definesFile(shaderDefines.c_str());
		std::string define;
		while (std::getline(definesFile, define)) {
			if (!define.empty() && define.rfind("//", 0) != 0) {
				description->addCompilerDefine(define.c_str());
			}
		}
	}

	description->setVertexShader(vertexShader.c_str());
	description->setFragmentShader(fragmentShader.c_str());
	description->setGeometryShader(geometryShader.c_str());

	for (auto item : defaultUniformSemantics) {
		description->setUniformSemantic(item.first.c_str(), item.second);
	}
	return description;
}

bool parseShaderText(ramses::Scene &scene, const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, const std::string &shaderDefines, raco::core::PropertyInterfaceList &outUniforms, raco::core::PropertyInterfaceList &outAttributes, std::string &outError) {
	outUniforms.clear();
	outAttributes.clear();
	auto description = createEffectDescription(vertexShader, geometryShader, fragmentShader, shaderDefines);
	ramses::Effect *effect = scene.createEffect(*description, ramses::ResourceCacheFlag_DoNotCache, "glsl shader");
	bool success = false;
	if (effect) {
		uint32_t numUniforms = effect->getUniformInputCount();
		for (uint32_t i{0}; i < numUniforms; i++) {
			ramses::UniformInput uniform;
			effect->getUniformInput(i, uniform);
			if (uniform.getSemantics() == ramses::EEffectUniformSemantic::Invalid) {
				if (shaderTypeMap.find(uniform.getDataType()) != shaderTypeMap.end()) {
					outUniforms.emplace_back(std::string{uniform.getName()}, shaderTypeMap[uniform.getDataType()]);
				} else {
					outError += std::string(uniform.getName()) + " has unsupported type  ";
				}
			}
		}

		uint32_t numAttributes = effect->getAttributeInputCount();
		for (uint32_t i{0}; i < numAttributes; i++) {
			ramses::AttributeInput attrib;
			effect->getAttributeInput(i, attrib);
			if (shaderTypeMap.find(attrib.getDataType()) != shaderTypeMap.end()) {
				outAttributes.emplace_back(std::string(attrib.getName()), shaderTypeMap[attrib.getDataType()]);
			} else {
				outError += std::string(attrib.getName()) + " has unsupported type  ";
			}
		}

		success = true;
		scene.destroy(*effect);
	} else {
		outError = scene.getLastEffectErrorMessages();
	}
	return success;
}

namespace {
void fillLuaScriptInterface(std::vector<raco::core::PropertyInterface> &interface, const rlogic::Property *property) {
	static const std::map<rlogic::EPropertyType, raco::core::EnginePrimitive> typeMap = {
		{rlogic::EPropertyType::Float, raco::core::EnginePrimitive::Double},
		{rlogic::EPropertyType::Vec2f, raco::core::EnginePrimitive::Vec2f},
		{rlogic::EPropertyType::Vec3f, raco::core::EnginePrimitive::Vec3f},
		{rlogic::EPropertyType::Vec4f, raco::core::EnginePrimitive::Vec4f},
		{rlogic::EPropertyType::Int32, raco::core::EnginePrimitive::Int32},
		{rlogic::EPropertyType::Vec2i, raco::core::EnginePrimitive::Vec2i},
		{rlogic::EPropertyType::Vec3i, raco::core::EnginePrimitive::Vec3i},
		{rlogic::EPropertyType::Vec4i, raco::core::EnginePrimitive::Vec4i},
		{rlogic::EPropertyType::String, raco::core::EnginePrimitive::String},
		{rlogic::EPropertyType::Bool, raco::core::EnginePrimitive::Bool},
		{rlogic::EPropertyType::Struct, raco::core::EnginePrimitive::Struct},
		{rlogic::EPropertyType::Array, raco::core::EnginePrimitive::Array}};
	interface.reserve(property->getChildCount());
	for (int i{0}; i < property->getChildCount(); i++) {
		auto child{property->getChild(i)};
		if (typeMap.find(child->getType()) != typeMap.end()) {
			// has children
			auto &it = interface.emplace_back(std::string{child->getName()}, typeMap.at(child->getType()));
			if (child->getChildCount() > 0) {
				fillLuaScriptInterface(it.children, child);
			}
		}
	}
}
}  // namespace

bool parseLuaScript(LogicEngine &engine, const std::string &luaScript, raco::core::PropertyInterfaceList &outInputs, raco::core::PropertyInterfaceList &outOutputs, std::string &outError) {
	auto script = engine.createLuaScriptFromSource(luaScript, "Stage::PreprocessScript");
	if (script) {
		if (auto inputs = script->getInputs()) {
			fillLuaScriptInterface(outInputs, inputs);
		}
		if (auto outputs = script->getOutputs()) {
			fillLuaScriptInterface(outOutputs, outputs);
		}
		engine.destroy(*script);
		return true;
	} else {
		outError = engine.getErrors().at(0).message;
		return false;
	}
}

ramses::RamsesVersion getRamsesVersion() {
	return ramses::GetRamsesVersion();
}

rlogic::RamsesLogicVersion getLogicEngineVersion() {
	return rlogic::GetRamsesLogicVersion();
}

std::string getRamsesVersionString() {
	return getRamsesVersion().string;
}

std::string getLogicEngineVersionString() {
	return std::string(getLogicEngineVersion().string);
}

void enableLogicLoggerOutputToStdout(bool enabled) {
	rlogic::Logger::SetDefaultLogging(enabled);
}

}  // namespace raco::ramses_base
