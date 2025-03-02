/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Project.h"
#include "core/PathManager.h"
#include "core/ExternalReferenceAnnotation.h"
#include "utils/PathUtils.h"

#include <cctype>
#include "utils/stdfilesystem.h"
namespace raco::core {

bool Project::removeInstances(std::set<SEditorObject> const& objects, bool gcExternalProjectMap) {
	for (const auto& object : objects) {
		instances_.erase(std::find(instances_.begin(), instances_.end(), object));
		instanceMap_.erase(object->objectID());
	}
	if (gcExternalProjectMap) {
		return gcExternalProjectMapping();
	}
	return false;
}

void Project::addInstance(SEditorObject object) {
	instances_.push_back(object);
	instanceMap_[object->objectID()] = object;
}

const std::vector<SEditorObject>& Project::instances() const {
	return instances_;
}

std::string Project::projectName() const {
	if (auto settingsObj = settings()) {
		return settingsObj->objectName();
	}
	return std::string();
}

std::string Project::projectID() const {
	if (auto settingsObj = settings()) {
		return settingsObj->objectID();
	}
	return std::string();
}

std::string Project::getProjectNameForObject(SEditorObject const& object, bool fallbackToLocalProject) const {
	if (auto extrefAnno = object->query<ExternalReferenceAnnotation>()) {
		return externalProjectsMap_.at(*extrefAnno->projectID_).name;
	}
	if (fallbackToLocalProject) {
		return projectName();
	}
	return std::string();
}

std::string Project::currentFolder() const {
	return folder_;
}

std::string Project::currentPath() const {
	return (std::filesystem::path(currentFolder()) / currentFileName()).generic_string();
}

std::string Project::currentFileName() const {
	return filename_;
}

void Project::setCurrentPath(const std::string& newPath) {
	if (utils::path::isExistingDirectory(newPath)) {
		folder_ = newPath;
		filename_.clear();
	} else {
		auto path = PathManager::normal_path(newPath);
		folder_ = path.parent_path().generic_string();
		filename_ = path.filename().string();
	}
}

void Project::addLink(SLink link) {
	linkStartPoints_[link->startObject_.asRef()->objectID()].insert(link);
	linkEndPoints_[link->endObject_.asRef()->objectID()].insert(link);
	links_.push_back(link);
	linkGraph_.addLink(link);
}

SLink Project::findLinkByObjectID(SLink link) const {
	auto linkEndObjID = (*link->endObject_)->objectID();

	auto linkEndPointIt = linkEndPoints_.find(linkEndObjID);

	if (linkEndPointIt != linkEndPoints_.end()) {
		auto& endPointLinks = linkEndPointIt->second;
		for (const auto& endPointLink : endPointLinks) {
			if (compareLinksByObjectID(*endPointLink, *link)) {
				return endPointLink;
			}
		}
	}

	return nullptr;
}

void Project::removeLink(SLink link) {
	linkGraph_.removeLink(link);

	const auto& startObjID = link->startObject_.asRef()->objectID();
	linkStartPoints_[startObjID].erase(link);
	if (linkStartPoints_[startObjID].empty()) {
		linkStartPoints_.erase(startObjID);
	}

	const auto& endObjID = link->endObject_.asRef()->objectID();
	linkEndPoints_[endObjID].erase(link);
	if (linkEndPoints_[endObjID].empty()) {
		linkEndPoints_.erase(endObjID);
	}

	links_.erase(std::find(links_.begin(), links_.end(), link));
}

const std::map<std::string, std::set<SLink>>& Project::linkStartPoints() const {
	return linkStartPoints_;
}

const std::map<std::string, std::set<SLink>>& Project::linkEndPoints() const {
	return linkEndPoints_;
}

const std::vector<SLink>& Project::links() const {
	return links_;
}

std::shared_ptr<const ProjectSettings> Project::settings() const {
	for (auto& instance : instances_) {
		if (instance->getTypeDescription().typeName == core::ProjectSettings::typeDescription.typeName)
			return std::dynamic_pointer_cast<core::ProjectSettings>(instance);
	}
	return {};
}

std::shared_ptr<ProjectSettings> Project::settings() {
	for (auto &instance : instances_) {
		if (instance->getTypeDescription().typeName == core::ProjectSettings::typeDescription.typeName)
			return std::dynamic_pointer_cast<core::ProjectSettings>(instance);
	}
	return {};
}

SEditorObject Project::getInstanceByID(const std::string& objectID) const {
	auto it = instanceMap_.find(objectID);
	if (it != instanceMap_.end()) {
		return it->second;
	}
	return SEditorObject{};
}

bool Project::createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const {
	return linkGraph_.createsLoop(start, end);
}

void Project::addExternalProjectMapping(const std::string& projectID, const std::string& absPath, const std::string& projectName) {
	if (projectID.empty()) {
		throw ExtrefError("External project with empty name not allowed.");
	}
	if (projectID == this->projectID()) {
		throw ExtrefError("External reference project loop detected (based on project ID).");
	}

	if (absPath == currentPath()) {
		throw ExtrefError("External reference project loop detected (based on project path).");
	}

	auto relPath = PathManager::constructRelativePath(absPath, currentFolder());

	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		if (relPath != it->second.path) {
			throw ExtrefError("Duplicate external project name with different file paths.");
		}
	}

	{
		auto it = std::find_if(externalProjectsMap_.begin(), externalProjectsMap_.end(), [&relPath](const auto& item) {
			return item.second.path == relPath;
		});
		if (it != externalProjectsMap_.end()) {
			if (it->first != projectID) {
				throw ExtrefError(fmt::format("Project ID change for file '{}' detected: '{}' renamed to '{}'", relPath, it->first, projectID));
			}
		}
	}

	externalProjectsMap_[projectID] = serialization::ExternalProjectInfo{relPath, projectName};
}

void Project::updateExternalProjectName(const std::string& projectID, const std::string& projectName) {
	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		it->second.name = projectName;
	}
}

void Project::removeExternalProjectMapping(const std::string& projectID) {
	externalProjectsMap_.erase(projectID);
}

bool Project::hasExternalProjectMapping(const std::string& projectID) const {
	return externalProjectsMap_.find(projectID) != externalProjectsMap_.end();
}

std::string Project::lookupExternalProjectPath(const std::string& projectID) const {
	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		return PathManager::constructAbsolutePath(currentFolder(), it->second.path);
	}
	return std::string();
}

std::string Project::lookupExternalProjectName(const std::string& projectID) const {
	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		return it->second.name;
	}
	return std::string();
}

void Project::rerootExternalProjectPaths(const std::string oldFolder, const std::string newFolder) {
	for (auto& item : externalProjectsMap_) {
		item.second.path = PathManager::rerootRelativePath(item.second.path, oldFolder, newFolder);
	}
}

bool Project::usesExternalProjectByPath(const std::string& absPath) const {
	for (auto item : externalProjectsMap_) {
		if (core::PathManager::constructAbsolutePath(currentFolder(), item.second.path) == absPath) {
			return true;
		}
	}
	return false;
}

const std::map<std::string, serialization::ExternalProjectInfo>& Project::externalProjectsMap() const {
	return externalProjectsMap_;  
}

bool Project::gcExternalProjectMapping() {
	std::set<std::string> usedExtProjects;
	for (auto obj : instances_) {
		if (auto extrefAnno = obj->query<ExternalReferenceAnnotation>()) {
			usedExtProjects.insert(*extrefAnno->projectID_);
		}
	}

	bool changed = false;
	auto it = externalProjectsMap_.begin();
	while (it != externalProjectsMap_.end()) {
		auto projectID = it->first;
		if (usedExtProjects.find(projectID) == usedExtProjects.end()) {
			it = externalProjectsMap_.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	return changed;
}

bool Project::externalReferenceUpdateFailed() const {
	return externalReferenceUpdateFailed_;
}

void Project::setExternalReferenceUpdateFailed(bool status) {
	externalReferenceUpdateFailed_ = status;
}


Project::LinkGraph::LinkGraph(const Project& project) {
	for (auto &link : project.links()) {
		addLink(link);
	}
}

void Project::LinkGraph::addLink(SLink link) {
	graph[*link->startObject_].insert(*link->endObject_);
}

void Project::LinkGraph::removeLink(SLink link) {
	auto it = graph.find(*link->startObject_);
	if (it != graph.end()) {
		it->second.erase(*link->endObject_);
		if (it->second.empty()) {
			graph.erase(it);
		}
	}
}

bool Project::LinkGraph::createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const {
	auto startObj = start.object();
	auto endObj = end.object();
	if (startObj == endObj) {
		return true;
	}
	if (graph.find(endObj) != graph.end()) {
		std::set<SEditorObject> visited;
		return depthFirstSearch(endObj, startObj, visited);
	}
	return false;
}

bool Project::LinkGraph::depthFirstSearch(SEditorObject current, SEditorObject obj, std::set<SEditorObject>& visited) const {
	if (current == obj) {
		return true;
	}
	if (visited.find(current) != visited.end()) {
		return false;
	}
	auto it = graph.find(current);
	if (it != graph.end()) {
		for (auto depObj : it->second) {
			if (depthFirstSearch(depObj, obj, visited)) {
				return true;
			}
		}
	}
	visited.insert(current);
	return false;
}

}  // namespace raco::core