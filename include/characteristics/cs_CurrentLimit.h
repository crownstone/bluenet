/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 2 Dec., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include "drivers/cs_LPComp.h"
#include "events/cs_Listener.h"

class CurrentLimit: public Listener {
public:
	CurrentLimit();

	~CurrentLimit();

	void init();
	void start(uint8_t* limit_value);

	// We get a dispatch from the LP comparator
	void handleEvent();
	void handleEvent(uint8_t type);
};
