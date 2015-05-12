/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */

#include <events/cs_EventDispatcher.h>

#include <cstddef>

#include <drivers/cs_Serial.h>

void EventDispatcher::dispatch(uint16_t evt) {
	dispatch(evt, NULL, 0);
}

void EventDispatcher::dispatch(uint16_t evt, void* p_data, uint16_t length) {
	LOGi("dispatch event: %d", evt);
	for (int i = 0; i < MAX_EVENT_LISTENERS; i++) {
		if (_listeners[i] != NULL &&
			(_listeners[i]->getType() == evt || _listeners[i]->getType() == EVT_ALL))
		{
			_listeners[i]->handleEvent(evt, p_data, length);
		}
	}
}

bool EventDispatcher::addListener(EventListener *listener) {
	LOGi("addListener for event: %d", listener->getType());
	for (int i = 0; i < MAX_EVENT_LISTENERS; i++) {
		if (_listeners[i] == NULL) {
			_listeners[i] = listener;
			return true;
		}
	}
	return false;
}

void EventDispatcher::removeListener(EventListener *listener) {
	LOGi("remove listener");
	for (int i = 0; i < MAX_EVENT_LISTENERS; i++) {
		if (_listeners[i] == listener) {
			_listeners[i] = NULL;
		}
	}
}
