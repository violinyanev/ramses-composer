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

#include "user_types/BaseCamera.h"

namespace raco::user_types {

class PerspectiveFrustum : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = {"PerspectiveFrustum", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	PerspectiveFrustum() : ClassWithReflectedMembers(getProperties()) {}

	PerspectiveFrustum(const PerspectiveFrustum& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: ClassWithReflectedMembers(getProperties()),
		  near_(other.near_),
		  far_(other.far_),
		  fov_(other.fov_),
		  aspect_(other.aspect_) {
	}

	PerspectiveFrustum& operator=(const PerspectiveFrustum& other) {
		near_ = other.near_;
		far_ = other.far_;
		fov_ = other.fov_;
		aspect_ = other.aspect_;
		return *this;
	}

	void copyAnnotationData(const PerspectiveFrustum& other) {
		near_.copyAnnotationData(other.near_);
		far_.copyAnnotationData(other.far_);
		fov_.copyAnnotationData(other.fov_);
		aspect_.copyAnnotationData(other.aspect_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"nearPlane", &near_},
			{"farPlane", &far_},
			{"fieldOfView", &fov_},
			{"aspectRatio", &aspect_}};
	}

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> near_{0.1, DisplayNameAnnotation("Near Plane"), RangeAnnotation<double>(0.1, 1.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> far_{1000.0, DisplayNameAnnotation("Far Plane"), RangeAnnotation<double>(100.0, 10000.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> fov_{35.0, DisplayNameAnnotation("Field of View"), RangeAnnotation<double>(10.0, 120.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> aspect_{1440.0 / 720.0, DisplayNameAnnotation("Aspect"), RangeAnnotation<double>(0.5, 4.0), {}};
};

class PerspectiveCamera : public BaseCamera {
public:
	static inline const TypeDescriptor typeDescription = {"PerspectiveCamera", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	PerspectiveCamera(PerspectiveCamera const& other)
		: BaseCamera(other), frustum_(other.frustum_) {
		fillPropertyDescription();
	}

	PerspectiveCamera(const std::string& name, const std::string& id) : BaseCamera(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("frustum", &frustum_);
	}

	Property<PerspectiveFrustum, DisplayNameAnnotation, LinkEndAnnotation> frustum_{{}, {"Frustum"}, {}};
};

}  // namespace raco::user_types