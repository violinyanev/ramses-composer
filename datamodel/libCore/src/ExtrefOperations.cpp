/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/Errors.h"
#include "core/ExtrefOperations.h"
#include "core/Context.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"

#include "log_system/log.h"

#include <algorithm>
#include <cassert>

namespace raco::core {

namespace {

struct ExternalObjectDescriptor {
	SEditorObject obj;
	Project* project;
};

// @exception ExtrefError
Project* lookupExternalProject(Project* project, const std::string& projectID, ExternalProjectsStoreInterface& externalProjectsStore, std::vector<std::string>& pathStack) {
	Project* extProject = nullptr;
	if (project->hasExternalProjectMapping(projectID)) {
		auto extPath = project->lookupExternalProjectPath(projectID);

		extProject = externalProjectsStore.addExternalProject(extPath, pathStack);
		if (extProject) {
			auto loadedID = extProject->projectID();
			if (loadedID != projectID) {
				throw core::ExtrefError(fmt::format("Project ID change for file '{}' detected: '{}' changed to '{}'", extPath, projectID, loadedID));
			}
		}
	}
	return extProject;
}

// @exception ExtrefError
ExternalObjectDescriptor lookupExtrefSource(Project* project, const ExternalObjectDescriptor& descriptor, ExternalProjectsStoreInterface& externalProjectsStore, std::vector<std::string>& pathStack) {
	auto anno = descriptor.obj->query<ExternalReferenceAnnotation>();
	if (anno) {
		auto sourceProjectID = *anno->projectID_;

		if (descriptor.project->hasExternalProjectMapping(sourceProjectID)) {
			auto path = descriptor.project->lookupExternalProjectPath(sourceProjectID);
			auto name = descriptor.project->lookupExternalProjectName(sourceProjectID);
			project->addExternalProjectMapping(sourceProjectID, path, name);
		}

		Project* sourceProject = lookupExternalProject(project, sourceProjectID, externalProjectsStore, pathStack);
		if (!sourceProject) {
			auto path = descriptor.project->lookupExternalProjectPath(sourceProjectID);
			throw ExtrefError("Can't load external project '" + sourceProjectID + "' with path '" + path + "'");
		}
		project->updateExternalProjectName(sourceProjectID, sourceProject->projectName());
		auto sourceObject = sourceProject->getInstanceByID(descriptor.obj->objectID());

		return {sourceObject, sourceProject};
	}
	return descriptor;
}

// @exception ExtrefError
void collectExternalObjects(Project* project, const ExternalObjectDescriptor& descriptor, ExternalProjectsStoreInterface& externalProjectsStore, std::map<std::string, ExternalObjectDescriptor>& externalObjects, std::vector<std::string>& pathStack, bool discardNonRoots) {
	auto it = externalObjects.find(descriptor.obj->objectID());
	if (it == externalObjects.end()) {
		auto sourceDesc = lookupExtrefSource(project, descriptor, externalProjectsStore, pathStack);
		if (sourceDesc.obj) {
			if (sourceDesc.obj->getParent() && discardNonRoots) {
				return;
			}
			// Check for pasting already existing local object from external project.
			// Only fails if the external project id has been renamed by hand in the project file.
			// Without that the external project would be recognized before as a duplicate of the local project.
			auto localObj = project->getInstanceByID(descriptor.obj->objectID());
			if (localObj && !localObj->query<ExternalReferenceAnnotation>()) {
				throw ExtrefError("Can't add existing local object as external reference.");
			}

			externalObjects[descriptor.obj->objectID()] = sourceDesc;

			for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(sourceDesc.obj))) {
				if (prop.type() == PrimitiveType::Ref) {
					auto refValue = prop.asTypedRef<EditorObject>();
					if (refValue) {
						collectExternalObjects(project, ExternalObjectDescriptor{refValue, sourceDesc.project}, externalProjectsStore, externalObjects, pathStack, false);
					}
				}
			}

			if (!sourceDesc.obj->getParent()) {
				// Follow links from endpoint -> starting point
				auto it = sourceDesc.project->linkEndPoints().find(sourceDesc.obj->objectID());
				if (it != sourceDesc.project->linkEndPoints().end()) {
					for (auto link : it->second) {
						collectExternalObjects(project, ExternalObjectDescriptor{*link->startObject_, sourceDesc.project}, externalProjectsStore, externalObjects, pathStack, false);
					}
				}
			}
		}
	} else {
		// If we find object by id make sure this is really from the same project (project id & path)
		auto sourceDesc = lookupExtrefSource(project, descriptor, externalProjectsStore, pathStack);
		if (sourceDesc.project->projectID() != it->second.project->projectID() ||
			sourceDesc.project->currentPath() != it->second.project->currentPath()) {
			throw ExtrefError(fmt::format("Duplicate object found: '{}' found in '{}' and '{}'.", sourceDesc.obj->objectName(), sourceDesc.project->currentPath(), it->second.project->currentPath()));
		}
	}
};

SLink lookupLink(SLink srcLink, const std::map<std::string, std::set<SLink>>& destLinks) {
	auto it = destLinks.find((*srcLink->endObject_)->objectID());
	if (it != destLinks.end()) {
		for (const auto& destLink : it->second) {
			if (compareLinksByObjectID(*srcLink, *destLink)) {
				return destLink;
			}
		}
	}
	return nullptr;
}


}  // namespace

void ExtrefOperations::updateExternalObjects(BaseContext& context, Project* project, ExternalProjectsStoreInterface& externalProjectsStore, std::vector<std::string>& pathStack) {
	// remove project-global errors
	context.errors().removeError(ValueHandle());

	DataChangeRecorder localChanges;
	auto extProjectMapCopy = project->externalProjectsMap();

	// local = collect all extref objects in current project
	std::set<SEditorObject> localObjects;
	std::copy_if(project->instances().begin(), project->instances().end(), std::inserter(localObjects, localObjects.end()), [](SEditorObject object) -> bool {
		return object->query<ExternalReferenceAnnotation>() != nullptr;
	});

	// remote = collect current state of extref objects in external projects
	// walk tree following all references but use objects from correct external project when following references.
	std::map<std::string, ExternalObjectDescriptor> externalObjects;

	try {
		for (auto object : localObjects) {
			if (object->getParent() == nullptr) {
				collectExternalObjects(project, ExternalObjectDescriptor{object, project}, externalProjectsStore, externalObjects, pathStack, true);
			}
		}
	} catch (const ExtrefError& e) {
		// Cleanup external projects that have been added by collectExternalObjects above but that are not used since we abort here.
		project->gcExternalProjectMapping();
		project->setExternalReferenceUpdateFailed(true);

		auto errorMsg = fmt::format("External reference update failed: {}", e.what());
		context.errors().addError(ErrorCategory::EXTERNAL_REFERENCE_ERROR, ErrorLevel::ERROR, ValueHandle(), errorMsg);
		LOG_ERROR(log_system::COMMON, errorMsg);

		throw e;
	}

	auto translateToLocal = [&localObjects](SEditorObject extObj) -> SEditorObject {
		if (extObj) {
			auto it = std::find_if(localObjects.begin(), localObjects.end(), [extObj](SEditorObject obj) {
				return obj->objectID() == extObj->objectID();
			});
			if (it != localObjects.end()) {
				return *it;
			}
		}
		return nullptr;
	};

	// Perform external -> local update

	// Delete locate objects
	std::vector<SEditorObject> toRemove;
	{
		auto it = localObjects.begin();
		while (it != localObjects.end()) {
			if (externalObjects.find((*it)->objectID()) == externalObjects.end()) {
				toRemove.emplace_back(*it);
				it = localObjects.erase(it);
			} else {
				++it;
			}
		}
	}
	// We need a full deleteObjects here in order to remove references to deleted extref objects in the local objects,
	// e.g. local material referencing extref texture with the extref update deleting the extref texture
	context.deleteObjects(toRemove, false);

	// Remove links
	std::map<std::string, std::set<SLink>> localLinks = Queries::getLinksConnectedToObjects(*project, localObjects, true, true);

	std::map<std::string, std::set<SLink>> externalLinks;
	for (const auto& item : externalObjects) {
		auto extObj = item.second.obj;
		auto extProject = item.second.project;
		// try to avoid duplicates:
		auto it = extProject->linkEndPoints().find(extObj->objectID());
		if (it != extProject->linkEndPoints().end()) {
			externalLinks[extObj->objectID()].insert(it->second.begin(), it->second.end());
		}
	}

	for (const auto& localLinksCont : localLinks) {
		for (const auto& localLink : localLinksCont.second) {
			if (!lookupLink(localLink, externalLinks)) {
				project->removeLink(localLink);
				localChanges.recordRemoveLink(localLink->descriptor());
			}
		}
	}

	// Create local objects
	for (auto item : externalObjects) {
		SEditorObject extObj = item.second.obj;
		if (!translateToLocal(extObj)) {
			auto localObj = context.objectFactory()->createObject(extObj->getTypeDescription().typeName, extObj->objectName(), extObj->objectID());
			localObj->addAnnotation(std::make_shared<ExternalReferenceAnnotation>(item.second.project->projectID()));
			context.project()->addInstance(localObj);
			localChanges.recordCreateObject(localObj);
			localObjects.insert(localObj);
		}
	}

	// Update properties
	for (auto item : externalObjects) {
		auto extObj = item.second.obj;
		auto localObj = translateToLocal(extObj);

		updateEditorObject(
			extObj.get(), localObj, translateToLocal, [](const std::string&) { return false; }, *context.objectFactory(), &localChanges, true, false);
	}

	// Create links
	for (const auto& extLinkCont : externalLinks) {
		for (const auto& extLink : extLinkCont.second) {
			auto localLink = lookupLink(extLink, localLinks);
			if (!localLink) {
				// create link
				auto localLink = Link::cloneLinkWithTranslation(extLink, translateToLocal);
				project->addLink(localLink);
				localChanges.recordAddLink(localLink->descriptor());
			} else if (*localLink->isValid_ != *extLink->isValid_) {
				// update link validity
				localLink->isValid_ = extLink->isValid_;
				localChanges.recordChangeValidityOfLink(localLink->descriptor());
			}
		}
	}

	project->gcExternalProjectMapping();

	// Update volatile data for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterDeserialization();
	}

	context.modelChanges().mergeChanges(localChanges);
	context.uiChanges().mergeChanges(localChanges);

	if (extProjectMapCopy != project->externalProjectsMap()) {
		context.modelChanges().recordExternalProjectMapChanged();
		context.uiChanges().recordExternalProjectMapChanged();
	}

	// Sync from external files for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterContextActivated(context);
		// This is necessary here although neither undo nor prefab update need it:
		// we have to call handlers for local objects referencing updated extref objects.
		context.callReferencedObjectChangedHandlers(destObj);
	}

	project->setExternalReferenceUpdateFailed(false);
}

}  // namespace raco::core
