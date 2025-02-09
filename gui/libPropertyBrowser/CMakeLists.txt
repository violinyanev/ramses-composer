#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

find_package(Qt5 COMPONENTS Widgets Test REQUIRED)

add_library(libPropertyBrowser
    include/property_browser/PropertyBrowserItem.h src/PropertyBrowserItem.cpp
    include/property_browser/PropertyBrowserLayouts.h
    include/property_browser/PropertyBrowserModel.h
    include/property_browser/PropertyBrowserRef.h src/PropertyBrowserRef.cpp
    include/property_browser/PropertyBrowserWidget.h src/PropertyBrowserWidget.cpp
    include/property_browser/PropertySubtreeChildrenContainer.h src/PropertySubtreeChildrenContainer.cpp
    include/property_browser/PropertySubtreeView.h src/PropertySubtreeView.cpp
    include/property_browser/WidgetFactory.h src/WidgetFactory.cpp

    include/property_browser/editors/BoolEditor.h src/editors/BoolEditor.cpp
    include/property_browser/editors/DoubleEditor.h src/editors/DoubleEditor.cpp
    include/property_browser/editors/EnumerationEditor.h src/editors/EnumerationEditor.cpp
    include/property_browser/editors/IntEditor.h src/editors/IntEditor.cpp
    include/property_browser/editors/LinkEditor.h src/editors/LinkEditor.cpp
    include/property_browser/editors/RefEditor.h src/editors/RefEditor.cpp
    include/property_browser/editors/StringEditor.h src/editors/StringEditor.cpp
    include/property_browser/editors/URIEditor.h src/editors/URIEditor.cpp
    include/property_browser/editors/VecNTEditor.h
    
    include/property_browser/controls/ExpandButton.h src/controls/ExpandButton.cpp
    include/property_browser/controls/MouseWheelGuard.h src/controls/MouseWheelGuard.cpp
    include/property_browser/controls/ScalarSlider.h src/controls/ScalarSlider.cpp
    include/property_browser/controls/SpinBox.h src/controls/SpinBox.cpp

    src/ErrorBox.h src/ErrorBox.cpp
)

target_include_directories(libPropertyBrowser
    PUBLIC
        include/
)
enable_warnings_as_errors(libPropertyBrowser)

set_target_properties(libPropertyBrowser PROPERTIES AUTOMOC TRUE)
set_target_properties(libPropertyBrowser PROPERTIES AUTORCC TRUE)
set_target_properties(libPropertyBrowser PROPERTIES AUTOUIC TRUE)

target_link_libraries(libPropertyBrowser
PUBLIC
    raco::Core
    raco::UserTypes
    raco::Components
    raco::CommonWidgets
    Qt5::Widgets
PRIVATE
    raco::Style
)

add_library(raco::PropertyBrowser ALIAS libPropertyBrowser)

if(PACKAGE_TESTS)
    add_subdirectory(tests)
endif()