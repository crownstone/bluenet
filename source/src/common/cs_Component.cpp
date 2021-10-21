/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 19, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Component.h>

// -------------------


void Component::setParent(Component* p) {
	_parent = p;
}

void Component::parentAllChildren() {
	for (auto child : getChildren()) {
		if (child != nullptr) {
			child->setParent(this);
		}
	}
}

cs_ret_code_t Component::initChildren() {
	parentAllChildren();

	for (Component* child : getChildren()) {
		if(child == nullptr) {
			continue;
		}

		LOGd("init child: 0x%x", child);

		if (auto retval = child->init() != ERR_SUCCESS) {
			return retval;
		}
	}

	return ERR_SUCCESS;
}
