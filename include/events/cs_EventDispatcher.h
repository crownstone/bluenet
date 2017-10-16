/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventTypes.h>
#include <events/cs_EventListener.h>

#define MAX_EVENT_LISTENERS 20

/**
 * Event dispatcher.
 */
class EventDispatcher {

private:
	EventDispatcher();

	//! This class is singleton, deny implementation
	EventDispatcher(EventDispatcher const&);
	//! This class is singleton, deny implementation
	void operator=(EventDispatcher const &);

	//! Array of listeners
	EventListener* _listeners[MAX_EVENT_LISTENERS];

	//! Count of added listeners
	uint16_t _listenerCount;

public:
	static EventDispatcher& getInstance() {
		static EventDispatcher instance;
		return instance;
	}

	//! Add a listener
	bool addListener(EventListener *listener);

	//! Dispatch an event without data
	void dispatch(uint16_t evt);

	//! Dispatch an event with data
	void dispatch(uint16_t evt, void* p_data, uint16_t length);
};


