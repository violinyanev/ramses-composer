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

#include "data_storage/ReflectionInterface.h"
#include "data_storage/Value.h"
#include "data_storage/Table.h"
#include "data_storage/BasicAnnotations.h"

#include <memory>
#include <set>
#include <stack>
#include <iterator>

namespace raco::core {

using namespace raco::data_storage;

class BaseContext;
class ValueHandle;
class Errors;

class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;
using WEditorObject = std::weak_ptr<EditorObject>;
using SCEditorObject = std::shared_ptr<const EditorObject>;


// This is the base class for complex objects useable as reference types in the Value class, 
// i.e. for PrimitiveType::Ref Values
//
// Types of data/member variables in EditorObjects
// - persistent data: Value<T> and Property<T,Annos...> member variables
//   These will be serialized.
//   Must not be declared mutable.
// - volatile data: all other members variables
//   These will not be serialized and need to be recreated after load/paste and similar operations.
//   May be declared mutable.
//
class EditorObject : public ClassWithReflectedMembers, public std::enable_shared_from_this<EditorObject> {
public:
	virtual bool serializationRequired() const override {
		return true;
	}
	EditorObject(EditorObject const& other) :
		ClassWithReflectedMembers(std::vector<std::pair<std::string, ValueBase*>>{}),
		objectID_(other.objectID_), objectName_(other.objectName_), children_(other.children_) {
		fillPropertyDescription();
	}

	EditorObject(std::string name = std::string(), std::string id = std::string());


	template<class C>
	std::shared_ptr<C> as() {
		return std::dynamic_pointer_cast<C>(shared_from_this());
	}

	std::string const& objectID() const;
	void setObjectID(std::string const& id);

	std::string const& objectName() const;
	void setObjectName(std::string const& name);

	
	struct ChildIterator {
		ChildIterator(SEditorObject const& object, size_t index);

		SEditorObject operator*();
		bool operator!=(ChildIterator const& other) const;
		ChildIterator& operator++();

	private:
		SEditorObject object_ = nullptr;
		size_t index_ = 0;
	};

	ChildIterator begin();
	ChildIterator end();

	// Find index of object in data object children of current object.
	// Returns -1 if not found among children.
	int findChildIndex(const EditorObject* object);
	
	// Get data model parent; root objects have nullptr as parent.
	SEditorObject getParent();


	//
	// Const handlers
	// - are declared 'const'
	// - purpose: initialize and maintain volatile data
	// - they are not allowed to change persistent data.
	//

	virtual void onAfterDeserialization() const;

	// Called when
	//   sourceReferenceProperty contains a reference to *this
	// before either
	// - sourceReferenceProperty is set different value(for primType_Ref Values)
	//   -> example: prefab instance template parent reference
	// - sourceReferenceProperty is removed from a Table
	//   -> example: scenegraph children table/array
	// - used to maintain backpointers to the referencing object
	virtual void onBeforeRemoveReferenceToThis(ValueHandle const& sourceReferenceProperty) const;

	// Called after either
	// - sourceReferenceProperty was set to *this (for primType_Ref Values)
	//   -> example: prefab instance template parent reference
	// - sourceReferenceProperty containing *this was added to a Table
	//   -> example: scenegraph children table/array
	// - used to maintain backpointers to the referencing object
	virtual void onAfterAddReferenceToThis(ValueHandle const& sourceReferenceProperty) const;

	// Called before objects are deleted via the Context.
	// - Should not perform any data model modifications.
	// - Can release volatile resources associated to the object like file watchers.
	// - Needed because cleanup via destructor will run too late since UI will in general hold
	//   shared pointers for longer than the desired lifetime of the resources, i.e. file watchers.
	virtual void onBeforeDeleteObject(Errors& errors) const;



	// 
	// Side-effect handlers for persistent data
	// - are not declared 'const' and take BaseContext parameter
	// - purpose: maintain consistency of the persistent data.
	// - are allowed to change persistent data.
	// - need to use contexts for modifying persistent data.
	//

	// Called after a context was created for the Project containg the EditorObject
	// sync with external files
	virtual void onAfterContextActivated(BaseContext& context) {}

	// Called after changing a value inside this object, possibly nested multiple levels.
	virtual void onAfterValueChanged(BaseContext& context, ValueHandle const& value) {}

	// Called after any property in the changedObject changed and a property inside the current
	// object contains a reference property to changedObject.
	// - example: MeshNode update after either the Mesh or Material have changed.
	virtual void onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {}



	// Data model children (not the same as scenegraph children):
	// - model ownership, i.e. lifetime of child depends on parent
	// - have unique parent, i.e. can appear only once in tree
	// - used to model both scenegraph children of nodes and prefab/prefab instance resource 
	//   contents (offscreen buffers etc)
	Property<Table, ArraySemanticAnnotation, HiddenProperty> children_{{}, {}, {}};

	static std::string normalizedObjectID(std::string const& id);

private:
	friend class BaseContext;

	void fillPropertyDescription() {
		properties_.emplace_back("objectID", &objectID_);
		properties_.emplace_back("objectName", &objectName_);
		properties_.emplace_back("children", &children_);
	}


	Property<std::string, HiddenProperty> objectID_{ std::string(), HiddenProperty() };
	Property<std::string, DisplayNameAnnotation> objectName_;

	mutable WEditorObject parent_;
	
	// volatile
	mutable std::set<WEditorObject, std::owner_less<WEditorObject>> referencesToThis_;
};


class TreeIterator {
public:
	TreeIterator() = default;
	TreeIterator(SEditorObject object);

	SEditorObject operator*();
	TreeIterator& operator++();
	bool operator!=(TreeIterator const& other);

private:
	operator bool();

	struct Item {
		Item& operator++() {
			++current;
			return *this;
		}
		operator bool() const {
			return current != end;
		}

		EditorObject::ChildIterator current;
		EditorObject::ChildIterator end;
	};

	SEditorObject top_;
	std::stack<Item> stack_;
};


class TreeIteratorAdaptor {
public:
	TreeIteratorAdaptor(SEditorObject root) : object_(root) {}

	TreeIterator begin() {
		return TreeIterator(object_);
	}
	TreeIterator end() {
		return TreeIterator();
	}

private:
	SEditorObject object_;
};

}

template<>
struct std::iterator_traits<raco::core::EditorObject::ChildIterator> {
	using value_type = raco::core::SEditorObject;
	using iterator_category = std::forward_iterator_tag;
};

template<>
struct std::iterator_traits<raco::core::TreeIterator> {
	using value_type = raco::core::SEditorObject;
	using iterator_category = std::forward_iterator_tag;
};
