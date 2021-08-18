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
 * Using this class 'sibling components' can query for
 * eachothers presence in a unified way, without need
 * for static classes or other hard dependencies.
 */
class Component {
private:
	Component* _parent = nullptr;
	std::vector<Component*> _children;

	/**
	 * Removes all entries in _children equal to element.
	 */
	void erase(Component* element) {
		for (auto it = _children.begin(); it != _children.end();) {
			if (*it == element) {
				it = _children.erase(it);
			}
			else {
				++it;
			}
		}
	}

public:
	// ================== Constructors ==================

	/**
	 * Default construction leaves _children empty and assures no memory is claimed for that empty array.
	 */
	Component() { _children.shrink_to_fit(); }

	/**
	 * this constructor takes a list of Component pointers, deletes any nullptrs from that list
	 * and shrinks it to fit.
	 */
	Component(std::initializer_list<Component*> children) : _children(children) {
		erase(nullptr);
		_children.shrink_to_fit();

		for (auto child: children) {
			child->_parent = this;
		}
	}

	virtual ~Component() = default;

	// ================== Getters ==================

	/**
	 * Returns a component of type T* from _children, or
	 * owned by the parent of this component.
	 * If none-exists, a nullptr is returned.
	 */
	template <class T>
	T* getComponent(Component* requester = nullptr) {
		// jump up in hierarchy to parent.
		if (requester == nullptr) {
			if (_parent != nullptr) {
				// request from the parent to find a sibling.
				// keeping track of the original requester.
				return _parent->getComponent<T>(this);
			} else {
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

	std::vector<Component*> getChildren() {
		return _children;
	}

	// ============== Add/Remove components ===============

	/**
	 * adds a new child to this component, changing its parent to `this`.
	 */
	void addComponent(Component* child) {
		if (child != nullptr) {
			_children.push_back(child);
			_children.shrink_to_fit();
			child->_parent = this;
		}
	}

	void addComponents(std::initializer_list<Component*> children) {
		_children.insert(_children.end(), children.begin(), children.end());
		erase(nullptr);
		_children.shrink_to_fit();

		for (auto child : children) {
			child->_parent = this;
		}
	}

	/**
	 * removes the component from the _children and sets its parent to nullptr.
	 */
	void removeComponent(Component* c) {
		erase(c);
		_children.shrink_to_fit();
		c->_parent = nullptr;
	}

	// ============== life cycle events ===============

	/**
	 * components can implement this if they need to find sibling
	 * components or do specific initialization.
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
	cs_ret_code_t initChildren() {
		for (Component* child : _children) {
			LOGd("init children: 0x%x", child);
			if(auto retval = child->init() != ERR_SUCCESS) {
				return retval;
			}
		}
		return ERR_SUCCESS;
	}



};
