/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/OrthographicCameraAdaptor.h"

#include "ramses_adaptor/BaseCameraAdaptorHelpers.h"
#include "ramses_adaptor/SceneAdaptor.h"

#include "ramses_base/RamsesHandles.h"



namespace raco::ramses_adaptor {
OrthographicCameraAdaptor::OrthographicCameraAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::OrthographicCamera> editorObject)
	: SpatialAdaptor(sceneAdaptor, editorObject, raco::ramses_base::ramsesOrthographicCamera(sceneAdaptor->scene())),
	  cameraBinding_{raco::ramses_base::ramsesCameraBinding(this->ramsesObject(), & sceneAdaptor->logicEngine())},
	  viewportSubscription_{sceneAdaptor->dispatcher()->registerOnChildren({editorObject, {"viewport"}}, [this](auto) {
		  tagDirty();
	  })},
	  frustrumSubscription_{sceneAdaptor->dispatcher()->registerOnChildren({editorObject, {"frustum"}}, [this](auto) {
		  tagDirty();
	  })} {
}

OrthographicCameraAdaptor::~OrthographicCameraAdaptor() {
	sceneAdaptor_->setCamera(nullptr);
}

bool OrthographicCameraAdaptor::sync(core::Errors* errors) {
	SpatialAdaptor::sync(errors);
	BaseCameraAdaptorHelpers::sync(editorObject(), &ramsesObject(), cameraBinding_.get());
	ramsesObject().setFrustum(static_cast<float>(*editorObject()->frustum_->left_), static_cast<float>(*editorObject()->frustum_->right_), static_cast<float>(*editorObject()->frustum_->bottom_), static_cast<float>(*editorObject()->frustum_->top_), static_cast<float>(*editorObject()->frustum_->near_), static_cast<float>(*editorObject()->frustum_->far_));
	sceneAdaptor_->setCamera(&ramsesObject());
	// The logic engine will always set the entire struct even if there is a link for only one of the values, and use the default values in the binding
	// for the non-linked elements in the struct - so we need to also set the default values for the bindings.
	cameraBinding_->getInputs()->getChild("frustum")->getChild("nearPlane")->set(static_cast<float>(*editorObject()->frustum_->near_));
	cameraBinding_->getInputs()->getChild("frustum")->getChild("farPlane")->set(static_cast<float>(*editorObject()->frustum_->far_));
	cameraBinding_->getInputs()->getChild("frustum")->getChild("leftPlane")->set(static_cast<float>(*editorObject()->frustum_->left_));
	cameraBinding_->getInputs()->getChild("frustum")->getChild("rightPlane")->set(static_cast<float>(*editorObject()->frustum_->right_));
	cameraBinding_->getInputs()->getChild("frustum")->getChild("bottomPlane")->set(static_cast<float>(*editorObject()->frustum_->bottom_));
	cameraBinding_->getInputs()->getChild("frustum")->getChild("topPlane")->set(static_cast<float>(*editorObject()->frustum_->top_));
	tagDirty(false);
	return true;
}

void OrthographicCameraAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	logicNodes.push_back(cameraBinding_.get());
}

const rlogic::Property* OrthographicCameraAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector)
{
	using raco::user_types::Node;
	using raco::user_types::property_name;

	static std::map<std::string_view, std::string_view> propertyNameToFrustrumPropertyName{
		{ "nearPlane", "nearPlane" },
		{ "farPlane", "farPlane" },
		{ "leftPlane", "leftPlane" },
		{ "rightPlane", "rightPlane" },
		{ "bottomPlane", "bottomPlane" },
		{ "topPlane", "topPlane" },
	};
	if (auto p = BaseCameraAdaptorHelpers::getProperty(cameraBinding_.get(), propertyNamesVector)) {
		return p;
	}
	if (propertyNamesVector.size() == 1 && propertyNamesVector[0] == "frustum") {
		return cameraBinding_->getInputs()->getChild("frustum");
	}
	if (propertyNamesVector.size() == 2 && propertyNamesVector[0] == "frustum") {
		std::string propName = propertyNamesVector[1];
		if (propertyNameToFrustrumPropertyName.find(propName) != propertyNameToFrustrumPropertyName.end()) {
			auto const ramsesFrustrumProperties = cameraBinding_->getInputs()->getChild("frustum");
			assert(ramsesFrustrumProperties != nullptr);
			auto const ramsesFrustrumProperty = ramsesFrustrumProperties->getChild(propertyNameToFrustrumPropertyName.at(propName));
			assert(ramsesFrustrumProperty != nullptr);
			return ramsesFrustrumProperty;
		}
	}
	return SpatialAdaptor::getProperty(propertyNamesVector);
}

}  // namespace raco::ramses_adaptor