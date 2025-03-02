/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserItem.h"

#include "core/CoreFormatter.h"
#include "core/Errors.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"
#include "core/Queries.h"
#include "data_storage/BasicAnnotations.h"
#include "log_system/log.h"
#include "property_browser/PropertyBrowserRef.h"

using raco::log_system::PROPERTY_BROWSER;

using SDataChangeDispatcher = raco::components::SDataChangeDispatcher;

namespace raco::property_browser {

	// Lua start index counting at 1 instead of 0
constexpr size_t LUA_INDEX_OFFSET = 1;

PropertyBrowserItem::PropertyBrowserItem(
	core::ValueHandle valueHandle,
	SDataChangeDispatcher dispatcher,
	core::CommandInterface* commandInterface,
	PropertyBrowserModel *model,
	QObject* parent)
	: QObject{parent},
	  parentItem_{dynamic_cast<PropertyBrowserItem*>(parent)},
	  valueHandle_{std::move(valueHandle)},
	  subscription_{dispatcher->registerOn(valueHandle_, [this]() {
		  if (valueHandle_.isObject() || hasTypeSubstructure(valueHandle_.type())) {
			  syncChildrenWithValueHandle();
		  }
		  Q_EMIT valueChanged(valueHandle_);
	  })},
	  errorSubscription_{dispatcher->registerOnErrorChanged(valueHandle_, [this]() {
		  Q_EMIT errorChanged(valueHandle_);
	  })},

	  commandInterface_{commandInterface},
	  dispatcher_{dispatcher},
	  model_{model} {
	LOG_TRACE(PROPERTY_BROWSER, "PropertyBrowserItem() from: {}", valueHandle_);
	children_.reserve(static_cast<int>(valueHandle_.size()));
	for (int i = 0; i < valueHandle_.size(); i++) {
		if (!raco::core::Queries::isHidden(*project(), valueHandle_[i])) {
			children_.push_back(new PropertyBrowserItem{valueHandle_[i], dispatcher, commandInterface_, model_, this});
		}
	}
	if (!valueHandle_.isObject() && valueHandle_.type() == core::PrimitiveType::Ref) {
		refItem_ = new PropertyBrowserRef(valueHandle_, dispatcher_, commandInterface_, this);
	}
	if (raco::core::Queries::isValidLinkEnd(valueHandle_)) {
		if (auto link = raco::core::Queries::getLink(*project(), valueHandle_.getDescriptor())) {
			linkValidityChangeSub_ = dispatcher_->registerOnLinkValidityChange(
				[this](const core::LinkDescriptor& link) {
					auto endHandle = core::ValueHandle(link.end);
					if (endHandle && endHandle == valueHandle_) {
						updateLinkState();
					}
				});
		}

		linkLifecycleSub_ = dispatcher_->registerOnLinksLifeCycle([this](const core::LinkDescriptor& link) {
			if (valueHandle_) {
				auto endHandle = core::ValueHandle(link.end);
				if (endHandle && endHandle == valueHandle_) {
					linkValidityChangeSub_ = dispatcher_->registerOnLinkValidityChange(
						[this](const core::LinkDescriptor& link) {
							auto endHandle = core::ValueHandle(link.end);
							if (endHandle && endHandle == valueHandle_) {
								updateLinkState();
							}
						});

					updateLinkState();
				}
			} },
			[this](const core::LinkDescriptor& link) {
				if (valueHandle_) {
					auto endHandle = core::ValueHandle(link.end);
					if (endHandle && endHandle == valueHandle_) {
						linkValidityChangeSub_ = components::Subscription();
						updateLinkState();
					}
				}
			});
	}
}

void PropertyBrowserItem::updateLinkState() noexcept {
	if (valueHandle_) {
		Q_EMIT linkTextChanged(linkText().c_str());
		Q_EMIT linkStateChanged(linkState());
		Q_EMIT editableChanged(editable());
		for (auto* child_ : children_) {
			child_->updateLinkState();
		}
	}
}

core::PrimitiveType PropertyBrowserItem::type() const noexcept {
	return valueHandle_.type();
}

PropertyBrowserRef* PropertyBrowserItem::refItem() noexcept {
	return refItem_;
}

PropertyBrowserModel* PropertyBrowserItem::model() const noexcept {
	return model_;
}

const QList<PropertyBrowserItem*>& PropertyBrowserItem::children()
{
	return children_;
}

bool PropertyBrowserItem::hasError() const noexcept {
	return commandInterface_->errors().hasError(valueHandle_);
}

void PropertyBrowserItem::markForDeletion() {
	// prevent crashes caused by delayed subscription callbacks
	subscription_ = raco::components::Subscription{};
	errorSubscription_ = raco::components::Subscription{};
	linkValidityChangeSub_ = raco::components::Subscription{};
	linkLifecycleSub_ = raco::components::Subscription{};

	for (auto& child : children_) {
		child->markForDeletion();
	}

	this->deleteLater();
}

const core::ErrorItem& PropertyBrowserItem::error() const {
	return commandInterface_->errors().getError(valueHandle_);
}

size_t PropertyBrowserItem::size() noexcept {
	return children_.size();
}

std::string PropertyBrowserItem::displayName() const noexcept {
	if (auto displayNameAnno = query<core::DisplayNameAnnotation>()) {
		return *displayNameAnno->name_;
	}

	auto propName = valueHandle_.getPropName();
	if (propName.empty()) {
		auto* p = reinterpret_cast<PropertyBrowserItem*>(parent());
		auto it = std::find(p->children().begin(), p->children().end(), this);
		return "#" + std::to_string(std::distance(p->children().begin(), it) + LUA_INDEX_OFFSET);
	}
	return propName;
}

core::ValueHandle& PropertyBrowserItem::valueHandle() noexcept {
	return valueHandle_;
}

raco::core::Project* PropertyBrowserItem::project() const {
	return commandInterface_->project();
}

raco::components::SDataChangeDispatcher PropertyBrowserItem::dispatcher() const {
	return dispatcher_;
}

raco::core::EngineInterface& PropertyBrowserItem::engineInterface() const {
	return commandInterface_->engineInterface();
}

bool PropertyBrowserItem::editable() noexcept {
	return !core::Queries::isReadOnly(*commandInterface_->project(), valueHandle());
}

bool PropertyBrowserItem::showChildren() const {
	return children_.size() > 0 && expanded_;
}

bool PropertyBrowserItem::showControl() const {
	return !expanded_ || children_.size() == 0;
}


raco::core::Queries::LinkState PropertyBrowserItem::linkState() const noexcept {
	return core::Queries::linkState(*commandInterface_->project(), valueHandle_);
}

void PropertyBrowserItem::setLink(const core::ValueHandle& start) noexcept {
	commandInterface_->addLink(start, valueHandle_);
}

void PropertyBrowserItem::removeLink() noexcept {
	commandInterface_->removeLink(valueHandle_.getDescriptor());
}

std::string PropertyBrowserItem::linkText() const noexcept {
	if (auto link{raco::core::Queries::getLink(*commandInterface_->project(), valueHandle_.getDescriptor())}) {
		auto propertyPath = core::PropertyDescriptor(link->startObject_.asRef(), link->startPropertyNamesVector()).getPropertyPath();
		if (!link->isValid()) {
			propertyPath += " (broken)";
		}

		return propertyPath;
	}

	return "";
}

PropertyBrowserItem* PropertyBrowserItem::parentItem() const noexcept {
	return parentItem_;
}

bool PropertyBrowserItem::isRoot() const noexcept {
	return parentItem() == nullptr;
}

bool PropertyBrowserItem::hasCollapsedParent() const noexcept {
	if (isRoot()) {
		return false;
	} else {
		return !parentItem()->expanded() || parentItem()->hasCollapsedParent();
	}
}

PropertyBrowserItem* PropertyBrowserItem::findItemWithNoCollapsedParentInHierarchy() noexcept {
	PropertyBrowserItem* current = this;
	while (current->hasCollapsedParent()) {
		current = current->parentItem();
	}
	return current;
}

bool PropertyBrowserItem::expanded() const noexcept {
	return expanded_;
}

void PropertyBrowserItem::toggleExpanded() noexcept {
	expanded_ = !expanded_;
	// notify the view state accordingly
	Q_EMIT expandedChanged(expanded_);
	Q_EMIT showChildrenChanged(showChildren());
	Q_EMIT showControlChanged(showControl());
}

void PropertyBrowserItem::syncChildrenWithValueHandle() {
	// clear children
	{
		for (auto& child : children_) {
			child->markForDeletion();
		}
		children_.clear();
	}

	// create new children
	{
		children_.reserve(static_cast<int>(valueHandle_.size()));
		for (int i{0}; i < valueHandle_.size(); i++) {
			if (!raco::core::Queries::isHidden(*project(), valueHandle_[i])) {
				children_.push_back(new PropertyBrowserItem(valueHandle_[i], dispatcher_, commandInterface_, model_, this));
			}
		}
	}

	Q_EMIT childrenChanged(children_);

	// notify first not collapsed item about the structural changed
	{
		auto* visibleItem = findItemWithNoCollapsedParentInHierarchy();
		assert(visibleItem != nullptr);
		Q_EMIT visibleItem->childrenChangedOrCollapsedChildChanged();
	}

	// notify the view state accordingly
	if (children_.size() <= 1) {
		Q_EMIT expandedChanged(expanded());
		Q_EMIT showControlChanged(showControl());
		Q_EMIT showChildrenChanged(showChildren());
	}
}

std::string to_string(PropertyBrowserItem& item) {
	std::stringstream ss{};
	ss << "PropertyBrowserItem { type: ";
	ss << static_cast<int>(item.valueHandle_.type());
	ss << ", displayName: \"";
	ss << item.displayName();
	ss << "\"";
	if (!hasTypeSubstructure(item.valueHandle_.type())) {
		ss << ", value: ";
		switch (item.valueHandle_.type()) {
			case core::PrimitiveType::Double:
				ss << std::to_string(item.valueHandle_.as<double>());
				break;
		}
	}
	if (item.children_.size() > 0) {
		ss << ", children: [ ";
		for (auto& child : item.children_) {
			ss << to_string(*child);
		}
		ss << " ]";
	}
	ss << " }";
	return ss.str();
}

}  // namespace raco::property_browser
