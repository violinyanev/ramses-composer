#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libUserTypes
	include/user_types/BaseCamera.h
	include/user_types/BaseObject.h
	include/user_types/BaseTexture.h
	include/user_types/CubeMap.h src/CubeMap.cpp
	include/user_types/DefaultValues.h
	include/user_types/Enumerations.h src/Enumerations.cpp
	include/user_types/LuaScript.h src/LuaScript.cpp
	include/user_types/Material.h src/Material.cpp
	include/user_types/Mesh.h src/Mesh.cpp
	include/user_types/MeshNode.h src/MeshNode.cpp
	include/user_types/Node.h
	include/user_types/OrthographicCamera.h
	include/user_types/PerspectiveCamera.h
	include/user_types/Prefab.h src/Prefab.cpp
	include/user_types/PrefabInstance.h src/PrefabInstance.cpp
	include/user_types/Texture.h src/Texture.cpp
	include/user_types/EngineTypeAnnotation.h

	include/user_types/UserObjectFactory.h src/UserObjectFactory.cpp

	include/user_types/SyncTableWithEngineInterface.h src/SyncTableWithEngineInterface.cpp
	src/Validation.h
)

target_include_directories(libUserTypes PUBLIC include/)
enable_warnings_as_errors(libUserTypes)

target_link_libraries(libUserTypes
PUBLIC
	raco::DataStorage
	raco::Core
PRIVATE
	raco::Utils
)

add_library(raco::UserTypes ALIAS libUserTypes)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()