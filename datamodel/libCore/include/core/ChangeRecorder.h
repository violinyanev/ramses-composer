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

#include "Handles.h"
#include "Link.h"


#include <map>
#include <set>
#include <vector>

namespace raco::core {

class DataChangeRecorderInterface {
public:
	virtual void reset() = 0;

	virtual void recordCreateObject(SEditorObject const& object) = 0;
	virtual void recordDeleteObject(SEditorObject const& object) = 0;

	virtual void recordValueChanged(ValueHandle const& value) = 0;
	
	virtual void recordAddLink(const LinkDescriptor& link) = 0;
	virtual void recordChangeValidityOfLink(const LinkDescriptor& link) = 0;
	virtual void recordRemoveLink(const LinkDescriptor& link) = 0;

	virtual void recordErrorChanged(ValueHandle const& value) = 0;

	virtual void recordPreviewDirty(SEditorObject const& object) = 0;

	virtual void recordExternalProjectMapChanged() = 0;
};

class DataChangeRecorder : public DataChangeRecorderInterface {
public:
	void reset() override;

	void recordCreateObject(SEditorObject const& object) override;
	void recordDeleteObject(SEditorObject const& object) override;

	void recordValueChanged(ValueHandle const& value) override;

	void recordAddLink(const LinkDescriptor& link) override;
	void recordChangeValidityOfLink(const LinkDescriptor& link) override;
	void recordRemoveLink(const LinkDescriptor& link) override;

	void recordErrorChanged(ValueHandle const& value) override;

	void recordPreviewDirty(SEditorObject const& object) override;
	
	void recordExternalProjectMapChanged() override;

	/**
	 * #reset() with return of all chanages.
	 */
	DataChangeRecorder release();

	std::set<SEditorObject> const& getCreatedObjects() const;
	std::set<SEditorObject> const& getDeletedObjects() const;

	// Get the set of all changes Values
	// - added/removed properties inside Tables will be recorded as change of the Table Value.
	//   No separate add/remove property notification is generated.
	std::map<std::string, std::set<ValueHandle>> const& getChangedValues() const;

	bool hasValueChanged(const ValueHandle& handle) const;

	std::map<std::string, std::set<LinkDescriptor>> const& getAddedLinks() const;
	std::map<std::string, std::set<LinkDescriptor>> const& getValidityChangedLinks() const;
	std::map<std::string, std::set<LinkDescriptor>> const& getRemovedLinks() const;

	// Construct set of all objects that have been changed in some way, i.e.
	// that have been created of which contain a changed Value.
	std::set<SEditorObject> getAllChangedObjects(bool includePreviewDirty = false, bool includeLinkStart = false, bool includeLinkEnd = false) const;

	std::set<ValueHandle> getChangedErrors() const;

	std::set<SEditorObject> getPreviewDirtyObjects() const;

	bool externalProjectMapChanged() const;

	void mergeChanges(const DataChangeRecorder& other);

private:
	// private helper class that stores, accesses, updates and erases link descriptor entries with below-linear runtime complexity.
	class LinkMap {
	public:
		void clear() {
			linkMap_.clear();
		}

		// Try to find link, then erase it. Returns true when found and erased, false otherwise.
		bool eraseLink(const LinkDescriptor& link);

		// Iterate through the saved links and insert the link end point objects, depending on which should be included, in the "objects" set.
		void insertLinkEndPointObjects(bool includeLinkStart, bool includeLinkEnd, std::set<SEditorObject>& objects) const;

		// Inserts a link or updates it when it is already contained in the linkMap_.
		void insertOrUpdateLink(const LinkDescriptor& link);

		// Only update the link when it is saved in the LinkMap. Returns true if a saved link was updated, false otherwise.
		bool updateLinkIfSaved(const LinkDescriptor& link);

		const std::map<std::string, std::set<LinkDescriptor>>& savedLinks() const {
			return linkMap_;
		}

	private:
		// Link descriptors are stored with end object id as key
		std::map<std::string, std::set<LinkDescriptor>> linkMap_;
	};
	std::set<SEditorObject> createdObjects_;
	std::set<SEditorObject> deletedObjects_;
	
	std::map<std::string, std::set<ValueHandle>> changedValues_;

	LinkMap addedLinks_;
	LinkMap changedValidityLinks_;
	LinkMap removedLinks_;

	std::set<ValueHandle> changedErrors_;
	std::set<SEditorObject> previewDirty_;

	bool externalProjectMapChanged_ = false;
};

class MultiplexedDataChangeRecorder : public DataChangeRecorderInterface {
public:
	void addRecorder(DataChangeRecorderInterface* recorder);
	void removeRecorder(DataChangeRecorderInterface* recorder);

	void reset() override;

	void recordCreateObject(SEditorObject const& object) override;
	void recordDeleteObject(SEditorObject const& object) override;

	void recordValueChanged(ValueHandle const& value) override;

	void recordAddLink(const LinkDescriptor& link) override;
	void recordChangeValidityOfLink(const LinkDescriptor& link) override;
	void recordRemoveLink(const LinkDescriptor& link) override;

	void recordErrorChanged(ValueHandle const& value) override;

	void recordPreviewDirty(SEditorObject const& object) override;

	void recordExternalProjectMapChanged() override;

private:
	std::vector<DataChangeRecorderInterface*> recorders_;
};

}  // namespace raco::core