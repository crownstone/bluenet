/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 1 Dec., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>

#include <events/cs_EventTypes.h>

/**
 * Event listener.
 */
class EventListener {

private:

public:
	EventListener() {}

	virtual ~EventListener() {};

	//! handle events
	virtual void handleEvent(uint16_t evt, void* p_data, uint16_t length) = 0;
};
