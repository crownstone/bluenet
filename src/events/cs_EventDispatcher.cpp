/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */

#include <events/cs_EventDispatcher.h>

#include <util/cs_BleError.h>

#include <drivers/cs_Serial.h>

//#define PRINT_EVENTDISPATCHER_VERBOSE

EventDispatcher::EventDispatcher() : _listenerCount(0)
{

}

void EventDispatcher::dispatch(uint16_t evt) {
	dispatch(evt, NULL, 0);
}

void EventDispatcher::dispatch(uint16_t evt, void* p_data, uint16_t length) {
#ifdef PRINT_EVENTDISPATCHER_VERBOSE
	LOGi("dispatch event: %d", evt);
#endif

	for (int i = 0; i < _listenerCount; i++) {
		_listeners[i]->handleEvent(evt, p_data, length);
	}
}

bool EventDispatcher::addListener(EventListener *listener) {
	if (_listenerCount >= MAX_EVENT_LISTENERS - 1) {
		APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
		return false;
	}
#ifdef PRINT_EVENTDISPATCHER_VERBOSE
	LOGi("add listener: %u", _listenerCount);
#endif
	_listeners[_listenerCount++] = listener;
	return true;
}
