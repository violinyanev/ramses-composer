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

#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <functional>

namespace raco::core {
class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;
}  // namespace raco::core

namespace raco::data_storage {

using raco::core::EditorObject;
using raco::core::SEditorObject;

class AnnotationBase;

class ReflectionInterface;
class ClassWithReflectedMembers;
class Table;

class Vec2f;
class Vec3f;
class Vec4f;
class Vec2i;
class Vec3i;
class Vec4i;

enum class PrimitiveType {
	// Simple scalar types:
	Bool = 0,
	Int,
	Double,
	String,

	// References: hold a std::shared_ptr<T> where T is EditorObject or a subclass
	Ref,

	// Dictionary-like: holds a Table
	Table,

	// Complex c++ class types
	Vec2f,
	Vec3f,
	Vec4f,
	Vec2i,
	Vec3i,
	Vec4i,

	// Complex C++ class types; must be class derived from ClassWithReflectedMembers
	Struct
};


template<class T> struct AlwaysFalse : std::false_type {};

inline bool hasTypeSubstructure(PrimitiveType type) {
	return type >= PrimitiveType::Table;
}

template <typename T>
constexpr PrimitiveType primitiveType() {
	if constexpr (std::is_same<T, int>::value) {
		return PrimitiveType::Int;
	} else if constexpr (std::is_same<T, double>::value) {
		return PrimitiveType::Double;
	} else if constexpr (std::is_same<T, bool>::value) {
		return PrimitiveType::Bool;
	} else if constexpr (std::is_same<T, std::string>::value) {
		return PrimitiveType::String;
	} else if constexpr (std::is_same<T, Table>::value) {
		return PrimitiveType::Table;
	} else if constexpr (std::is_same<T, Vec2f>::value) {
		return PrimitiveType::Vec2f;
	} else if constexpr (std::is_same<T, Vec3f>::value) {
		return PrimitiveType::Vec3f;
	} else if constexpr (std::is_same<T, Vec4f>::value) {
		return PrimitiveType::Vec4f;
	} else if constexpr (std::is_same<T, Vec2i>::value) {
		return PrimitiveType::Vec2i;
	} else if constexpr (std::is_same<T, Vec3i>::value) {
		return PrimitiveType::Vec3i;
	} else if constexpr (std::is_same<T, Vec4i>::value) {
		return PrimitiveType::Vec4i;
	} else if constexpr (std::is_base_of<ClassWithReflectedMembers, T>::value) {
		return PrimitiveType::Struct;
	} else if constexpr (std::is_base_of<EditorObject, typename T::element_type>::value) {
		return PrimitiveType::Ref;
	} else {
		static_assert(AlwaysFalse<T>::value, "Unknown Type in Value!");
	}
}
std::string getTypeName(PrimitiveType type);
bool isPrimitiveTypeName(const std::string& type);
PrimitiveType toPrimitiveType(const std::string& type);

using TypeMapType = std::tuple<bool, int, double, std::string, SEditorObject, Table, Vec2f, Vec3f, Vec4f, Vec2i, Vec3i, Vec4i>;

// Variant with annotations
// - variant types:
//   - bool, int, float, string
//   - ReflectionInterface* (replaces reference container)
//   - Table (no pointer, replaces embedded container)
//   - vec3f, vec4f (embedded container)
//   - NO EditorObject
//   - can add further C++ classes derived from ClassWithReflectedMembers as PrimitiveType::Struct
//   - set of types is statically known at compile time; enum for type
// - type is static, can't be changed after creation; type mismatch in get/set operations will fail

// on assignment, i.e. ValueBase& operator=(ValueBase const&)
// - copies only the value but no annotation data
// - requires matching primitive types
// - performs deep copy
// - Table properties
//   have dynamic properties
//   -> assignment will add/remove properties & change types
// - class-type properties, e.g. Vec3f
//   static property set
//   -> only copy values

// On reference type Values (PrimitiveType::Ref)
// - reference type Values may store a pointer to a subclass of EditorObject, i.e.
//   Value<U> and Property<U, Args...> are allowed if U is subclass of EditorObject
// - Value::setRef and Value::canSetRef will enforce type compatibility:
//   if the argument pointer can't be dynamic_cast to the actual pointer type (U in Value<U>)
//   the assignment will fail with an exception and the check will return false.

// On Struct type Values (PrimitiveType::Struct)
// - Struct type Values store a subclass of ClassWithReflectedMembers as value (not as reference)
//   similar to the way the Vec2f etc types work
// - they are type safe: operations will enforce identical classes at runtime if
//   multiple PrimitiveType::Struct Values are involved.
// - for a Value<CC> to be usable the class CC must be derived from ClassWithReflectedMembers
// - CC must implement the following member functions (same as for e.g. Vec2f)
//   CC(const CC& other, std::function<SEditorObject(SEditorObject)>* translateRef)
//   CC& operator=(const CC& other) 
//   copyAnnotationData(const CC& other) 
//   std::vector<std::pair<std::string, ValueBase*>> getProperties()
// - Value::setStruct will enforce identical types at runtime using dynamic_cast and fail
//   with an exception if the types are not identical

class ValueBase {
public:
	static std::unique_ptr<ValueBase> create(PrimitiveType type);

	virtual PrimitiveType type() const = 0;
	virtual std::string typeName() const = 0;

	virtual bool& asBool() = 0;
	virtual int& asInt() = 0;
	virtual double& asDouble() = 0;
	virtual std::string& asString() = 0;
	virtual Table& asTable() = 0;

	virtual Vec2f& asVec2f() = 0;
	virtual Vec3f& asVec3f() = 0;
	virtual Vec4f& asVec4f() = 0;
	virtual Vec2i& asVec2i() = 0;
	virtual Vec3i& asVec3i() = 0;
	virtual Vec4i& asVec4i() = 0;

	virtual const bool& asBool() const = 0;
	virtual const int& asInt() const = 0;
	virtual const double& asDouble() const = 0;
	virtual const std::string& asString() const = 0;
	virtual const Table& asTable() const = 0;

	virtual const Vec2f& asVec2f() const = 0;
	virtual const Vec3f& asVec3f() const = 0;
	virtual const Vec4f& asVec4f() const = 0;
	virtual const Vec2i& asVec2i() const = 0;
	virtual const Vec3i& asVec3i() const = 0;
	virtual const Vec4i& asVec4i() const = 0;

	// Return a reference to the generic interface class for all
	// PrimitiveTypes that are not scalar types but contain substructure, i.e.
	// Tables and the Vec2f,... types.
	// Useful for generic iteration and visiting all scalar Values inside an object.
	// PrimitiveType::Ref is _not_ counted as having substructure.
	virtual ReflectionInterface& getSubstructure() = 0;
	virtual const ReflectionInterface& getSubstructure() const = 0;

	template <class T>
	T& as();
	template <class T>
	const T& as() const;

	// Get the SEditorObject base pointer for all Ref type Values.
	virtual SEditorObject asRef() const = 0;

	// Set the reference; this may fail if the pointer can not be dynamically downcast
	// to the actual type of the Value.
	virtual void setRef(SEditorObject) = 0;

	// Check if the current value is compatible with the argument pointer and could be set
	virtual bool canSetRef(SEditorObject) const = 0;

	// Get reference to generic base class for Struct type values.
	// Will not work for vector (Vec2f etc) types or Tables.
	virtual const ClassWithReflectedMembers& asStruct() const = 0;

	// Set Struct type values.
	// Identical types are dynamically enforced as runtime.
	virtual void setStruct(const ClassWithReflectedMembers&) = 0;

	// Check for equality of the actual classes of the arguments;
	// differs from comparing the PrimitiveType by
	// - Value<T> differs from Property<T>
	// - Property<T, ...> with different annotations classes differ
	// - different SEditorClass subclasses used as type in Value or Property differ
	static bool classesEqual(const ValueBase& left, const ValueBase& right); 

	// The assignment operator doesn't copy the annotation data; see notes above
	virtual ValueBase& operator=(const ValueBase&) = 0;

	// Assign value and return true if new value different from current value
	virtual bool assign(const ValueBase& newValue, bool includeAnnoData = false) = 0;

	// Compare the value but not the annotation data
	virtual bool operator==(const ValueBase&) const = 0;

	// Copy the annnotation data.
	virtual void copyAnnotationData(const ValueBase& src) = 0;

	ValueBase& operator=(bool value);
	ValueBase& operator=(int value);
	ValueBase& operator=(double value);
	ValueBase& operator=(std::string const& value);
	ValueBase& operator=(SEditorObject value);

	// Assignment function; needed because we can't template specialize operator= above
	template <typename T>
	ValueBase& set(T const& value);

	virtual std::unique_ptr<ValueBase> clone(std::function<SEditorObject(SEditorObject)>* translateRef) const = 0;

	static std::unique_ptr<ValueBase> from(bool value);
	static std::unique_ptr<ValueBase> from(double);

	template <class Anno>
	Anno* query() const {
		return dynamicQuery<Anno>();
	}

	template <class Anno>
	Anno* dynamicQuery() const {
		for (auto anno : baseAnnotationPtrs()) {
			if (Anno* p = dynamic_cast<Anno*>(anno)) {
				return p;
			}
		}
		return nullptr;
	}

	virtual const std::vector<AnnotationBase*>& baseAnnotationPtrs() const {
		static std::vector<AnnotationBase*> noAnnotations;
		return noAnnotations;
	}
};

template <>
inline ValueBase& ValueBase::set<SEditorObject>(SEditorObject const& value) {
	setRef(value);
	return *this;
}

template<typename T>
void primitiveCopyAnnotationData(T& dest, const T& src);


template <typename T>
class Value : public ValueBase {
public:
	Value() = default;
	Value(T const& val) : ValueBase(), value_(val) {}
	Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other, translateRef) {}

	virtual PrimitiveType type() const override;
	virtual std::string typeName() const override {
		if constexpr (std::is_same<T, SEditorObject>::value) {
			return getTypeName(primitiveType<T>());
		} else if constexpr (std::is_convertible<T, std::shared_ptr<ReflectionInterface>>::value) {
			return T::element_type::typeDescription.typeName;
		} else if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
			return T::typeDescription.typeName;
		} else {
			return getTypeName(primitiveType<T>());
		}
	}

	ValueBase& operator=(const ValueBase& other) override {
		if constexpr (std::is_convertible<T, SEditorObject>::value) {
			setRef(other.asRef());
		} else if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
			setStruct(other.asStruct());
		} else {
			value_ = other.as<T>();
		}
		return *this;
	}

	// Assign value and return true if new value different from current value
	bool assign(const ValueBase& other, bool includeAnnoData = false) override {
		if (includeAnnoData) {
			// TODO detect changes here
			// Undo currently ignores changes in annotations too.
			// OK like this until we have modifyable annotation data.
			copyAnnotationData(other);
		}
		if constexpr (std::is_convertible<T, SEditorObject>::value) {
			if (value_ != other.asRef()) {
				setRef(other.asRef());
				return true;
			}
		} else if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
			if (!(value_ == other.asStruct())) {
				setStruct(other.asStruct());
				return true;
			}
		} else {
			if (!(value_ == other.as<T>())) {
				value_ = other.as<T>();
				return true;
			}
		}
		return false;
	}

	bool operator==(const ValueBase& other) const override {
		if (classesEqual(*this, other)) {
			if constexpr (std::is_convertible<T, SEditorObject>::value) {
				return value_ == other.asRef();
			} else if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
				const T* vp = dynamic_cast<const T*>(&other.asStruct());
				if (!vp) {
					throw std::runtime_error("type mismatch");
				}
				return value_ == *vp;
			} else {
				return value_ == other.as<T>();
			}
		}
		return false;
	}

	virtual void copyAnnotationData(const ValueBase& src) override {
		if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
			const T* vp = dynamic_cast<const T*>(&src.asStruct());
			if (!vp) {
				throw std::runtime_error("type mismatch");
			}
			value_.copyAnnotationData(*vp);
		} else if constexpr (!std::is_convertible<T, SEditorObject>::value) {
			// The Vec2f,... types may have annotations nested inside, so we need to copy them too:
			primitiveCopyAnnotationData<T>(value_, src.as<T>());
		}
	}

	Value& operator=(Value const& other) {
		value_ = other.value_;
		return *this;
	}

	template <typename U>
	typename std::enable_if<std::is_convertible<U, T>::value, Value&>::type
	operator=(const Value<U>& other) {
		value_ = *other;
		return *this;
	}

	void operator=(T const& newValue) {
		value_ = newValue;
	}

	T& operator*() {
		return value_;
	}

	T const& operator*() const {
		return value_;
	}

	T* operator->() {
		return &value_;
	}

	T const* operator->() const {
		return &value_;
	}

	virtual bool& asBool() override {
		return asImpl<bool>();
	}
	virtual int& asInt() override {
		return asImpl<int>();
	}
	virtual double& asDouble() override {
		return asImpl<double>();
	}
	virtual std::string& asString() override {
		return asImpl<std::string>();
	}
	virtual SEditorObject asRef() const override {
		if constexpr (std::is_convertible<T, SEditorObject>::value) {
			return value_;
		}
		throw std::runtime_error("type mismatch");
	}
	virtual void setRef(SEditorObject v) override {
		if constexpr (std::is_same<T, SEditorObject>::value) {
			value_ = v;
		} else if constexpr (std::is_convertible<T, SEditorObject>::value) {
			if (v != nullptr) {
				auto p = std::dynamic_pointer_cast<typename T::element_type>(v);
				if (!p) {
					throw std::runtime_error("type mismatch");
				}
			}
			value_ = std::static_pointer_cast<typename T::element_type>(v);
		}
	}
	virtual const ClassWithReflectedMembers& asStruct() const override {
		if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
			return value_;
		}
		throw std::runtime_error("type mismatch");
	}
	virtual void setStruct(const ClassWithReflectedMembers& newValue) override {
		if constexpr (primitiveType<T>() == PrimitiveType::Struct) {
			const T* vp = dynamic_cast<const T*>(&newValue);
			if (!vp) {
				throw std::runtime_error("type mismatch");
			}
			value_ = *vp;
		}
	}

	virtual bool canSetRef(SEditorObject v) const override {
		if constexpr (std::is_same<T, SEditorObject>::value) {
			return true;
		} else if constexpr (std::is_convertible<T, SEditorObject>::value) {
			if (v != nullptr) {
				if (!std::dynamic_pointer_cast<typename T::element_type>(v)) {
					return false;
				}
			}
			return true;
		}
		return false;
	}

	virtual ReflectionInterface& getSubstructure() override {
		return asImpl<ReflectionInterface>();
	}
	virtual Table& asTable() override {
		return asImpl<Table>();
	}
	virtual Vec2f& asVec2f() override {
		return asImpl<Vec2f>();
	}
	virtual Vec3f& asVec3f() override {
		return asImpl<Vec3f>();
	}
	virtual Vec4f& asVec4f() override {
		return asImpl<Vec4f>();
	}
	virtual Vec2i& asVec2i() override {
		return asImpl<Vec2i>();
	}
	virtual Vec3i& asVec3i() override {
		return asImpl<Vec3i>();
	}
	virtual Vec4i& asVec4i() override {
		return asImpl<Vec4i>();
	}

	virtual const bool& asBool() const override {
		return asImplConst<bool>();
	}
	virtual const int& asInt() const override {
		return asImplConst<int>();
	}
	virtual const double& asDouble() const override {
		return asImplConst<double>();
	}
	virtual const std::string& asString() const override {
		return asImplConst<std::string>();
	}
	virtual const ReflectionInterface& getSubstructure() const override {
		return asImplConst<ReflectionInterface>();
	}
	virtual const Table& asTable() const override {
		return asImplConst<Table>();
	}
	virtual const Vec2f& asVec2f() const override {
		return asImplConst<Vec2f>();
	}
	virtual const Vec3f& asVec3f() const override {
		return asImplConst<Vec3f>();
	}
	virtual const Vec4f& asVec4f() const override {
		return asImplConst<Vec4f>();
	}
	virtual const Vec2i& asVec2i() const override {
		return asImplConst<Vec2i>();
	}
	virtual const Vec3i& asVec3i() const override {
		return asImplConst<Vec3i>();
	}
	virtual const Vec4i& asVec4i() const override {
		return asImplConst<Vec4i>();
	}

	virtual std::unique_ptr<ValueBase> clone(std::function<SEditorObject(SEditorObject)>* translateRef) const {
		if constexpr (std::is_same<T, SEditorObject>::value) {
			if (translateRef) {
				return std::make_unique<Value>((*translateRef)(**this));
			}
			return std::make_unique<Value>(*this);
		} else if constexpr (std::is_convertible<T, SEditorObject>::value) {
			if (translateRef) {
				auto p = std::dynamic_pointer_cast<typename T::element_type>((*translateRef)(**this));
				return std::make_unique<Value>(p);
			}
			return std::make_unique<Value>(*this);
		} else {
			return std::make_unique<Value>(*this, translateRef);
		}
	}

private:
	template <typename U>
	U& asImpl();

	template <typename U>
	const U& asImplConst() const;

	T value_;
};

template <typename T>
PrimitiveType Value<T>::type() const {
	return primitiveType<T>();
}

template <typename T>
template <typename U>
U& Value<T>::asImpl() {
	if constexpr (std::is_convertible<T&, U&>::value) {
		return value_;
	}
	throw std::runtime_error("type mismatch");
}
template <typename T>
template <typename U>
const U& Value<T>::asImplConst() const {
	if constexpr (std::is_convertible<T&, U&>::value) {
		return value_;
	}
	throw std::runtime_error("type mismatch");
}

template <typename T, class... Args>
class Property : public Value<T> {
public:
	static constexpr bool hasAnnotations() {
		return (sizeof...(Args) > 0);
	}

	virtual std::string typeName() const override {
		return std::string{(Value<T>::typeName() + ... + ("::" + Args::typeDescription.typeName))};
	}

	Property(Property const& other) : Value<T>(other), annotations_(other.annotations_) {
		buildAnnotationBasePointers();
	}

	Property(Property const&& other) = delete;

	Property() : Value<T>() {
		buildAnnotationBasePointers();
	}

	Property(const T& value, const Args&... args) : Value<T>(value), annotations_(args...) {
		buildAnnotationBasePointers();
	}

	virtual std::unique_ptr<ValueBase> clone(std::function<SEditorObject(SEditorObject)>* translateRef) const override {
		if constexpr (std::is_same<T, SEditorObject>::value) {
			if (translateRef) {
				return std::make_unique<Property>((*translateRef)(**this),
					std::get<Args>(annotations_)...);
			}
			return std::make_unique<Property>(*this);
		} else if constexpr (std::is_convertible<T, SEditorObject>::value) {
			if (translateRef) {
				auto p = std::dynamic_pointer_cast<typename T::element_type>((*translateRef)(**this));
				return std::make_unique<Property>(p, std::get<Args>(annotations_)...);
			}
			return std::make_unique<Property>(*this);
		} else {
			return std::make_unique<Property>(*this);
		}
	}

	Property& operator=(const Property& other) {
		Value<T>::operator=(other);
		return *this;
	}

	template <typename U>
	typename std::enable_if<std::is_convertible<U, T>::value, Property&>::type
	operator=(const Property<U>& other) {
		Value<T>::operator=(other);
		return *this;
	}

	virtual void copyAnnotationData(const ValueBase& src) override {
		Value<T>::copyAnnotationData(src);
		const Property* src_p = dynamic_cast<const Property*>(&src);
		annotations_ = src_p->annotations_;
	}

	void operator=(T const& newValue) {
		Value<T>::operator=(newValue);
	}

	template <class Anno>
	Anno* query() {
		return &staticQuery<Anno>();
	}

	template <class Anno>
	Anno& staticQuery() {
		return std::get<Anno>(annotations_);
	}

	const std::vector<AnnotationBase*>& baseAnnotationPtrs() const override {
		return baseAnnotationPtrs_;
	}

	void buildAnnotationBasePointers() {
		baseAnnotationPtrs_ = std::apply(
			[](Args&... annos) {
				return std::vector<AnnotationBase*>{static_cast<AnnotationBase*>(&annos)...};
			},
			annotations_);
	}

	std::tuple<Args...> annotations_;
	std::vector<AnnotationBase*> baseAnnotationPtrs_;
};


template <>
inline Value<bool>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
inline Value<int>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
inline Value<double>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
inline Value<std::string>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

}  // namespace raco::data_storage
