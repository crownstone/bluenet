/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventTypes.h>
#include <events/cs_EventListener.h>

#define MAX_EVENT_LISTENERS                            10

class EventDispatcher {

private:
	EventDispatcher() {};

	// This class is singleton, deny implementation
	EventDispatcher(EventDispatcher const&);
	// This class is singleton, deny implementation
	void operator=(EventDispatcher const &);

	EventListener* _listeners[MAX_EVENT_LISTENERS];

public:
	static EventDispatcher& getInstance() {
		static EventDispatcher instance;
		return instance;
	}

	// add a listener
	bool addListener(EventListener *listener);

	// remove one
	void removeListener(EventListener *listener);

	void dispatch(uint16_t evt);

	void dispatch(uint16_t evt, void* p_data, uint16_t length);
};


