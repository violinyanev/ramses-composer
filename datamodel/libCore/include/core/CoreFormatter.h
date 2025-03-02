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

#include "core/EditorObject.h"
#include "core/Handles.h"
#include "core/Link.h"
#include "core/EngineInterface.h"
#include <spdlog/fmt/fmt.h>
#include <sstream>

template <>
struct fmt::formatter<raco::data_storage::PrimitiveType> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const raco::data_storage::PrimitiveType type, FormatContext& ctx) {
		string_view name = std::to_string(static_cast<int>(type));
		switch (type) {
			case raco::data_storage::PrimitiveType::Bool:
				name = "Bool";
				break;
			case raco::data_storage::PrimitiveType::Double:
				name = "Double";
				break;
			case raco::data_storage::PrimitiveType::Int:
				name = "Int";
				break;
			case raco::data_storage::PrimitiveType::Ref:
				name = "Ref";
				break;
			case raco::data_storage::PrimitiveType::String:
				name = "String";
				break;
			case raco::data_storage::PrimitiveType::Table:
				name = "Table";
				break;
			case raco::data_storage::PrimitiveType::Vec2f:
				name = "Vec2f";
				break;
			case raco::data_storage::PrimitiveType::Vec2i:
				name = "Vec2i";
				break;
			case raco::data_storage::PrimitiveType::Vec3f:
				name = "Vec3f";
				break;
			case raco::data_storage::PrimitiveType::Vec3i:
				name = "Vec3i";
				break;
			case raco::data_storage::PrimitiveType::Vec4f:
				name = "Vec4f";
				break;
			case raco::data_storage::PrimitiveType::Vec4i:
				name = "Vec4i";
				break;
		}
		return formatter<string_view>::format(name, ctx);
	}
};


template <>
struct fmt::formatter<raco::core::EnginePrimitive> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const raco::core::EnginePrimitive type, FormatContext& ctx) {
		static const std::unordered_map<raco::core::EnginePrimitive, std::string_view> nameMap = {
			{raco::core::EnginePrimitive::Undefined, "Undefined"},
			{raco::core::EnginePrimitive::Bool, "Bool"},
			{raco::core::EnginePrimitive::Int32, "Int"},
			{raco::core::EnginePrimitive::UInt16, "Int"},
			{raco::core::EnginePrimitive::UInt32, "Int"},
			{raco::core::EnginePrimitive::Double, "Float"},
			{raco::core::EnginePrimitive::String, "String"},
			{raco::core::EnginePrimitive::Vec2f, "Vec_2f"},
			{raco::core::EnginePrimitive::Vec3f, "Vec_3f"},
			{raco::core::EnginePrimitive::Vec4f, "Vec_4f"},
			{raco::core::EnginePrimitive::Vec2i, "Vec_2i"},
			{raco::core::EnginePrimitive::Vec3i, "Vec_3i"},
			{raco::core::EnginePrimitive::Vec4i, "Vec_4i"},
			{raco::core::EnginePrimitive::Struct, "Struct"},
			{raco::core::EnginePrimitive::Array, "Arrray"},
			{raco::core::EnginePrimitive::TextureSampler2D, "TextureSampler2D"},
			{raco::core::EnginePrimitive::TextureSampler3D, "TextureSampler3D"},
			{raco::core::EnginePrimitive::TextureSamplerCube, "TextureSamplerCube"}};
		return formatter<string_view>::format(nameMap.at(type), ctx);
	}
};

template <>
struct fmt::formatter<raco::core::ValueHandle> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const raco::core::ValueHandle& c, FormatContext& ctx) {
		if (!c) {
			return formatter<string_view>::format(fmt::format("ValueHandle[Project-Global]"), ctx);
		}
		if (c.isObject()) {
			return formatter<string_view>::format(fmt::format("ValueHandle[Object]( objectName: {} )", c.rootObject()->objectName()), ctx);
		}
		return formatter<string_view>::format(fmt::format("ValueHandle[{}]( propName: {} )", c.type(), c.getPropName()), ctx);
	}
};

template <>
struct fmt::formatter<std::vector<raco::core::ValueHandle>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const std::vector<raco::core::ValueHandle>& c, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("{}", fmt::join(c, ", ")), ctx);
	}
};

struct ObjectNameOnly {
	raco::core::SEditorObject obj;
};
template <>
struct fmt::formatter<ObjectNameOnly> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ObjectNameOnly& c, FormatContext& ctx) {
		if (!c.obj) {
			return fmt::format_to(ctx.out(), "nullptr");
		}
		return fmt::format_to(ctx.out(), "{}", c.obj->objectName());
	}
};

template <>
struct fmt::formatter<raco::core::SLink> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const raco::core::SLink& link, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "Link {{ start: {}#{} end: {}#{} }}",
			ObjectNameOnly{*link->startObject_},
			fmt::join(link->startProp_->asVector<std::string>(), "."),
			ObjectNameOnly{*link->endObject_},
			fmt::join(link->endProp_->asVector<std::string>(), "."));
	}
};

template<>
struct fmt::formatter<raco::core::LinkDescriptor> : formatter<string_view> {
	template<typename FormatContext>
	auto format(const raco::core::LinkDescriptor& link, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "Link {{ start: {}#{} end: {}#{} }}",
			ObjectNameOnly{link.start.object()},
			fmt::join(link.start.propertyNames(), "."),
			ObjectNameOnly{link.end.object()},
			fmt::join(link.end.propertyNames(), "."));
	}
};

