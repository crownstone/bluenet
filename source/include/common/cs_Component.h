/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 18, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <vector>

#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_Typedefs.h>

#include <logging/cs_Logger.h>

// REVIEW: this class seems like a lot of RAM/code overhead for its current use case.

/**
 * Helper class to manage decoupling of components.
 *
 * Features that have many parts, can be organized in components
 * E.g.:
 *
 * AssetFiltering
 * - FilterSyncer
 * - AssetStore
 * - AssetFilterStore
 * - AssetForwarder
 *
 * These parts may depend on each other, but we don't want to expose
 * their existance to the whole firmware through global/static variables.
 *
 * Using this class 'sibling components' can query for
 * each others presence in a unified way, without need
 * for static classes or other rigid dependencies.
 */
class Component {
private:
	Component* _parent = nullptr;
	std::vector<Component*> _children;

public:
	// ================== Constructors ==================

	/**
	 * Leaves _children empty and shrinks the _children vector to capacity 0.
	 */
	Component();

	/**
	 * Takes a brace enclosed list of Component pointers,
	 *  - deletes any nullptrs from that list,
	 *  - parents the component to `this`,
	 *  - and shrinks it to fit.
	 */
	Component(std::initializer_list<Component*> children);

	virtual ~Component() = default;

	// ================== Getters ==================

	inline std::vector<Component*> getChildren() {
		return _children;
	}

	/**
	 * Returns a component of type T* from _children, or
	 * owned by the parent of this component.
	 * If none-exists, a nullptr is returned.
	 */
	template <class T>
	T* getComponent(Component* requester = nullptr);


	// ============== Add/Remove components ===============

	/**
	 * adds a new child to this component, changing its parent to `this`.
	 */
	void addComponent(Component* child);

	void addComponents(std::initializer_list<Component*> children);


	/**
	 * removes the component from the _children and sets its parent to nullptr.
	 */
	void removeComponent(Component* c);

	// ============== life cycle events ===============

	/**
	 * Components can implement this if they need to get references to sibling
	 * or if they need to do specific initialization.
	 *
	 * - Components are responsible for call init() on their children.
	 * - init is allowed to assume all siblings are constructed.
	 *
	 * E.g.
	 *
	 * class componentX : public Component {
	 * public:
	 * 		cs_ret_code_t init() {
	 * 			// construct childA
	 * 			...
	 * 			// construct childZ
	 *
	 * 			return initChildren();
	 * 		}
	 * };
	 */
	virtual cs_ret_code_t init() { return ERR_SUCCESS; }

	/**
	 * Initializes all children in the order that they were added to this component.
	 * Stops as soon as a child returns != ERR_SUCCESS and returns that error code.
	 *
	 * Components are not required to use this. They can call all inits of children
	 * in custom order if they need to. (And implement elegant failure)
	 */
	cs_ret_code_t initChildren();

private:
	/**
	 * Removes all entries in _children equal to element.
	 */
	void erase(Component* element);
};

// -------------------- Template implementation details -------------------

template <class T>
T* Component::getComponent(Component* requester) {
	// jump up in hierarchy to parent.
	if (requester == nullptr) {
		if (_parent != nullptr) {
			// request from the parent to find a sibling.
			// keeping track of the original requester.
			return _parent->getComponent<T>(this);
		}
		else {
			// no parent means no siblings.
			// use get subcomponents if you want that.
			return nullptr;
		}
	}

	// traverse children
	for (auto c : _children) {
		if (c == requester) {
			// skip original requester to avoid infinite recursions.
			continue;
		}

		T* t = dynamic_cast<T*>(c);
		if (t != nullptr) {
			return t;
		}
	}

	// jump up in hierarchy one stack frame deeper
	if (_parent != nullptr) {
		return _parent->getComponent<T>(this);
	}

	return nullptr;
}
