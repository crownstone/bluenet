/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventDispatcher.h>
#include <events/cs_EventListener.h>

void EventListener::listen() {
	EventDispatcher::getInstance().addListener(this);
}

EventListener::~EventListener() {
	EventDispatcher::getInstance().removeListener(this);
}
