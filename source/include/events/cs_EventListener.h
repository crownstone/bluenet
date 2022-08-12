/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 1 Dec., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_Event.h>

#include <cstdint>

/**
 * Event listener.
 */
class EventListener {
public:
	EventListener() {}

	virtual ~EventListener(){};

	/** Handle events
	 *
	 * This method is overloaded by all classes that derive from EventListener. They can receive an event_t struct
	 * and act upon it. These events are dispatched by the EventDispatcher.
	 */
	virtual void handleEvent(event_t& event) = 0;

	/**
	 * Registers this with the EventDispatcher.
	 */
	void listen();
};
