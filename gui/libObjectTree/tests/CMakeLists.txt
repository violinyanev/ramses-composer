#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
set(TEST_SOURCES 
    ObjectTreeDockManager_test.h ObjectTreeDockManager_test.cpp
    ObjectTreeNode_test.cpp
    ObjectTreeViewExternalProjectModel_test.cpp
    ObjectTreeViewDefaultModel_test.h ObjectTreeViewDefaultModel_test.cpp
    ObjectTreeViewMultipleModels_test.h ObjectTreeViewMultipleModels_test.cpp
)

set(TEST_LIBRARIES
    raco::ObjectTree
    raco::ApplicationLib
    raco::RamsesBase
    raco::Testing
)

raco_package_add_gui_test(
    libObjectTree_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_add_test_resouces(
    libObjectTree_test "${CMAKE_SOURCE_DIR}/resources"
)
