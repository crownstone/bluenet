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
	EventType _type;

public:
	EventListener(EventType type) {
		_type = type;
	}

	inline EventType getType() { return _type; }

	virtual ~EventListener() {};

	// handle events
	virtual void handleEvent(EventType evt, void* p_data, uint16_t length) = 0;
};
