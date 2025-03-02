/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/PathManager.h"
#include "utils/PathUtils.h"

#include <algorithm>
#include <cctype>

namespace raco::core {

std::filesystem::path PathManager::basePath_;

std::filesystem::path PathManager::normal_path(const std::string& path) {
	return raco_filesystem_compatibility::lexically_normal(std::filesystem::path(path));
}

void PathManager::init(const std::string& executableDirectory) {
	basePath_ = normal_path(executableDirectory).parent_path().parent_path();
}

std::filesystem::path PathManager::defaultBaseDirectory() {
	return basePath_;
}

std::string PathManager::defaultConfigDirectory() {
	return (defaultBaseDirectory() / DEFAULT_CONFIG_SUB_DIRECTORY).generic_string();
}

std::filesystem::path PathManager::defaultResourceDirectory() {
	return defaultBaseDirectory() / DEFAULT_PROJECT_SUB_DIRECTORY;
}

std::string PathManager::logFilePath() {
	return (std::filesystem::path(defaultConfigDirectory()) / LOG_FILE_NAME).generic_string();
}

std::string PathManager::layoutFilePath() {
	return (std::filesystem::path(defaultConfigDirectory()) / Q_LAYOUT_FILE_NAME).generic_string();
}

std::string PathManager::recentFilesStorePath() {
	return (std::filesystem::path(defaultConfigDirectory()) / Q_RECENT_FILES_STORE_NAME).generic_string();
}

std::string PathManager::preferenceFileLocation() {
	return (std::filesystem::path(defaultConfigDirectory()) / Q_PREFERENCES_FILE_NAME).generic_string();
}

std::string PathManager::defaultProjectFallbackPath() {
	return (defaultBaseDirectory() / DEFAULT_PROJECT_SUB_DIRECTORY).generic_string();
}

std::string PathManager::constructRelativePath(const std::string& absolutePath, const std::string& basePath) {
	// use std::filesystem::proximate() call with std::error_code to prevent exceptions and return the input path instead.
	std::error_code ec;
	return std::filesystem::proximate(absolutePath, basePath, ec).generic_string();
}

std::string PathManager::constructAbsolutePath(const std::string& dirPath, const std::string& filePath) {
	if (std::filesystem::path(filePath).is_absolute()) {
		return raco_filesystem_compatibility::lexically_normal(std::filesystem::path(filePath)).generic_string();
	}
	return raco_filesystem_compatibility::lexically_normal(std::filesystem::path(dirPath) / filePath).generic_string();
}

std::string PathManager::rerootRelativePath(const std::string& relativePath, const std::string& oldPath, const std::string& newPath) {
	auto uriAbsolutePath = PathManager::constructAbsolutePath(oldPath, relativePath);
	return PathManager::constructRelativePath(uriAbsolutePath, newPath);
}

bool PathManager::pathsShareSameRoot(const std::string& lhd, const std::string& rhd) {
	auto leftRoot = std::filesystem::path(lhd).root_name().generic_string();
	auto rightRoot = std::filesystem::path(rhd).root_name().generic_string();
	std::transform(leftRoot.begin(), leftRoot.end(), leftRoot.begin(), tolower);
	std::transform(rightRoot.begin(), rightRoot.end(), rightRoot.begin(), tolower);

	return leftRoot == rightRoot;
}

const std::string& PathManager::getCachedPath(const std::string& key, const std::string& fallbackPath) {
	auto pathIt = cachedPaths_.find(key);
	if (pathIt != cachedPaths_.end()) {
		auto& cachedPath = pathIt->second;
		if (!raco::utils::path::isExistingDirectory(cachedPath) && !fallbackPath.empty()) {
			return fallbackPath;
		}
		return cachedPath;
	}

	return fallbackPath;
}

void PathManager::setCachedPath(const std::string& key, const std::string& path) {
	auto pathIt = cachedPaths_.find(key);
	if (pathIt != cachedPaths_.end()) {
		pathIt->second = path;
	}
}

void PathManager::setAllCachedPathRoots(const std::string& folder) {
	setCachedPath(DEFAULT_PROJECT_SUB_DIRECTORY, folder);
	setCachedPath(IMAGE_SUB_DIRECTORY, folder + "/" + IMAGE_SUB_DIRECTORY);
	setCachedPath(MESH_SUB_DIRECTORY, folder + "/" + MESH_SUB_DIRECTORY);
	setCachedPath(SCRIPT_SUB_DIRECTORY, folder + "/" + SCRIPT_SUB_DIRECTORY);
	setCachedPath(SHADER_SUB_DIRECTORY, folder + "/" + SHADER_SUB_DIRECTORY);
}

std::string PathManager::sanitizePath(const std::string& path) {
	const auto trimmedStringLeft = std::find_if_not(path.begin(), path.end(), [](auto c) { return std::isspace(c); });
	const auto trimmedStringRight = std::find_if_not(path.rbegin(), path.rend(), [](auto c) { return std::isspace(c); }).base();

	if (trimmedStringLeft >= trimmedStringRight) {
		return std::string();
	}

	auto sanitizedPathString = raco_filesystem_compatibility::lexically_normal(std::filesystem::path(trimmedStringLeft, trimmedStringRight)).generic_string();

	return sanitizedPathString;
}

}  // namespace raco::core
