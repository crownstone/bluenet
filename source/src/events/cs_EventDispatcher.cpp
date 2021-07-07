/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <events/cs_EventDispatcher.h>
#include <logging/cs_Logger.h>
#include <util/cs_BleError.h>

#define LOGEventdispatcherInfo LOGi
#define LOGEventdispatcherWarning LOGw

EventDispatcher::EventDispatcher() : _listenerCount(0) {}

void EventDispatcher::dispatch(event_t& event) {
	if (event.size != 0 && event.data == nullptr) {
		LOGe("data nullptr while size != 0");
		event.result.returnCode = ERR_BUFFER_UNASSIGNED;
		return;
	}

	switch (event.type) {
		case CS_TYPE::CMD_ADD_BEHAVIOUR:
		case CS_TYPE::CMD_REPLACE_BEHAVIOUR:
		case CS_TYPE::CMD_UPLOAD_FILTER:
			// These types have variable sized data, and will be size checked in the handler.
			break;
		default:
			if (event.size != TypeSize(event.type)) {
				LOGEventdispatcherWarning("Can't dispatch: wrong payload length (%u should be %u) for type %u",
						event.size, TypeSize(event.type), event.type);
				event.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
				return;
			}
	}

	for (int i = 0; i < _listenerCount; i++) {
		_listeners[i]->handleEvent(event);
	}
}

bool EventDispatcher::addListener(EventListener* listener) {
	if (_listenerCount >= MAX_EVENT_LISTENERS - 1) {
		APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
		return false;
	}
	if (listener == nullptr) {
		APP_ERROR_CHECK(NRF_ERROR_NULL);
		return false;
	}

	// check for duplicate registration
	for (uint8_t listenerIndex = 0; listenerIndex < _listenerCount; listenerIndex++) {
		if(_listeners[listenerIndex] == listener) {
			return true;
		}
	}

	_listeners[_listenerCount++] = listener;
	return true;
}
