/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 19, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Component.h>

Component::Component() {
	_children.shrink_to_fit();
}

Component::Component(std::initializer_list<Component*> children) : _children(children) {
	erase(nullptr);
	_children.shrink_to_fit();

	for (auto child : children) {
		child->_parent = this;
	}
}

void Component::erase(Component* element) {
	for (auto it = _children.begin(); it != _children.end();) {
		if (*it == element) {
			it = _children.erase(it);
		}
		else {
			++it;
		}
	}
}

void Component::addComponent(Component* child) {
	if (child != nullptr) {
		_children.push_back(child);
		_children.shrink_to_fit();
		child->_parent = this;
	}
}

void Component::addComponents(std::initializer_list<Component*> children) {
	_children.insert(_children.end(), children.begin(), children.end());
	erase(nullptr);
	_children.shrink_to_fit();

	for (auto child : children) {
		child->_parent = this;
	}
}

void Component::removeComponent(Component* c) {
	erase(c);
	_children.shrink_to_fit();
	c->_parent = nullptr;
}

cs_ret_code_t Component::initChildren() {
	for (Component* child : _children) {
		LOGd("init children: 0x%x", child);
		if (auto retval = child->init() != ERR_SUCCESS) {
			return retval;
		}
	}
	return ERR_SUCCESS;
}
