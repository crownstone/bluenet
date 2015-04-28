/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)                                                                       
 * Date: 1 Dec., 2014                                                                                                   
 * License: LGPLv3+, Apache License, or MIT, your choice                                                                
 */
#pragma once

#include <cstdint>

//#include <stdint.h>
//#include <common/cs_Types.h>

class Listener {
public:
	virtual ~Listener() {};

	// have a general listener
	virtual void handleEvent() = 0;

	// or specify different types
	virtual void handleEvent(uint8_t type) = 0;
};
