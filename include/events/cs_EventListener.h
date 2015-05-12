/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 1 Dec., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

#include <events/cs_EventTypes.h>

class EventListener {

private:
	uint16_t _type;

public:
	EventListener(uint16_t type = EVT_ALL) {
		_type = type;
	}

	inline uint16_t getType() { return _type; }

	virtual ~EventListener() {};

	// handle events
	virtual void handleEvent(uint16_t evt, void* p_data, uint16_t length) = 0;
};
