/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */

#include <events/cs_EventDispatcher.h>

#include <util/cs_BleError.h>

#include <drivers/cs_Serial.h>

//#include <algorithm>

void EventDispatcher::dispatch(uint16_t evt) {
	dispatch(evt, NULL, 0);
}

void EventDispatcher::dispatch(uint16_t evt, void* p_data, uint16_t length) {
//	LOGi("dispatch event: %d", evt);

//	std::vector<EventListener*>::iterator it = _listeners.begin();
//	for (std::vector<EventListener*>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
//		uint16_t type = (*it)->getType();
//		if (type == evt || type == EVT_ALL) {
//			(*it)->handleEvent(evt, p_data, length);
//		}
//	}

	for (int i = 0; i < MAX_EVENT_LISTENERS; i++) {
		if (_listeners[i] != NULL &&
			(_listeners[i]->getType() == evt || _listeners[i]->getType() == EVT_ALL))
		{
			_listeners[i]->handleEvent(evt, p_data, length);
		}
	}
}

bool EventDispatcher::addListener(EventListener *listener) {

//	LOGi("addListener for event: %d", listener->getType());

//	_listeners.push_back(listener);
//	_listeners.shrink_to_fit();

	for (int i = 0; i < MAX_EVENT_LISTENERS; i++) {
		if (_listeners[i] == NULL) {
			_listeners[i] = listener;
			LOGi(">>> addListener: %d", i+1);
			return true;
		}
	}
	APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
	return false;
}

void EventDispatcher::removeListener(EventListener *listener) {
//	LOGi("remove listener");

//	std::vector<EventListener*>::iterator it = find(_listeners.begin(), _listeners.end(), listener);
//	if (it != _listeners.end()) {
//		_listeners.erase(it);
//		_listeners.shrink_to_fit();
//	}

	for (int i = 0; i < MAX_EVENT_LISTENERS; i++) {
		if (_listeners[i] == listener) {
			_listeners[i] = NULL;
		}
	}
}
