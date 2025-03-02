/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ramses_adaptor/ObjectAdaptor.h"
#include "components/DataChangeDispatcher.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include <ramses-client-api/SceneObjectIterator.h>
#include <ramses-logic/LogicEngine.h>
#include <type_traits>

template <typename T = ramses::RamsesObject, bool StrictTypeMatch = true>
inline std::vector<T*> select(const ramses::Scene& scene, ramses::ERamsesObjectType type = ramses::ERamsesObjectType_RamsesObject) {
	std::vector<T*> result{};
	auto it = ramses::SceneObjectIterator{scene, type};
	while (auto object = it.getNext()) {
		if constexpr (StrictTypeMatch) {
			if (object->getType() == type) {
				result.push_back(static_cast<T*>(object));
			}
		} else {
			result.push_back(static_cast<T*>(object));
		}
	}
	return result;
}

template <typename T>
inline const T* select(const rlogic::LogicEngine& engine, const char* name) {
	if constexpr (std::is_same<rlogic::LuaScript, T>::value) {
		for (const auto& luaScript: engine.scripts()) {
			if (luaScript->getName() == name)
				return luaScript;
		}
	} else {
		static_assert(std::is_same<rlogic::LuaScript, T>::value);
	}
	return nullptr;
}

template <typename T = ramses::RamsesObject>
inline const T* select(const ramses::Scene& scene, const char* name) {
	return static_cast<const T*>(scene.findObjectByName(name));
}

inline std::vector<ramses::RamsesObject*> select_all(const ramses::Scene& scene) {
	return select<ramses::RamsesObject, false>(scene);
}

template <class BaseClass = ::testing::Test>
class RamsesBaseFixture : public TestEnvironmentCoreT<BaseClass> {
public:
	using DataChangeDispatcher = raco::components::DataChangeDispatcher;

	RamsesBaseFixture() : dataChangeDispatcher{std::make_shared<DataChangeDispatcher>()},
						  sceneContext{&this->backend.client(), &this->backend.logicEngine(), ramses::sceneId_t{1u}, &this->project, dataChangeDispatcher, &this->errors} {}

	std::shared_ptr<DataChangeDispatcher> dataChangeDispatcher;
	raco::ramses_adaptor::SceneAdaptor sceneContext;

	void dispatch() {
		dataChangeDispatcher->dispatch(this->recorder.release());
		sceneContext.logicEngine().update();
	}

protected:
	template <typename RamsesType>
	bool isRamsesNameInArray(const char* name, const std::vector<RamsesType*>& arrayOfArrays) {
		return std::find_if(arrayOfArrays.begin(), arrayOfArrays.end(), [name](RamsesType* array) {
			return std::strcmp(array->getName(), name) == 0;
		}) != arrayOfArrays.end();
	};
};
