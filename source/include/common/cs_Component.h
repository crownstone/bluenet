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
public:
	/**
	 * Returns a component of type T* from _parent->children(),
	 * If not found try again with ancestors: _parent-> ... ->_parent->children().
	 *
	 * If none-exists, a nullptr is returned.
	 *
	 * Usage: auto sibling = getComponent<SiblingComponentType>();
	 */
	template <class T>
	T* getComponent(Component* requester = nullptr);

	// ============== life cycle events ===============

	/**
	 * Components can implement this if they need to get references to sibling
	 * or if they need to do specific initialization.
	 *
	 * - Components are responsible for calling init() on their children.
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

	// ================== Con-/destructors ==================

	virtual ~Component() = default;

protected:
	// ================== Getters ==================

	/**
	 * Components with children can override this method to return
	 * them. This is used by getComponent<> to search for available
	 * components.
	 */
	virtual std::vector<Component*> getChildren() {
		return {};
	}

	// ============== parenting ==============

	/**
	 * - parent all non-nullptr children.
	 * - init all non-nullptr children.
	 *
	 * Components are not required to use this. They can call all inits of children
	 * in custom order if they need to. (And implement elegant failure.)
	 *
	 * Initialisation is in order of the elements of the returnvalue of getChildren.
	 */
	cs_ret_code_t initChildren();

 	/**
 	 * Children that are instantiated later can also be added
 	 * individually.
 	 */
	void setParent(Component* p);

private:
	Component* _parent = nullptr;
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
	for (auto c : getChildren()) {
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
