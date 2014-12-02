/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)                                                                       
 * Date: 1 Dec., 2014                                                                                                   
 * License: LGPLv3+, Apache License, or MIT, your choice                                                                
 */

#ifndef CS_LISTENER_H
#define CS_LISTENER_H

#include <stdint.h>

class Listener {
public:
	// have a general listener
	virtual void handleEvent() = 0;

	// or specify different types
	virtual void handleEvent(uint8_t type) = 0;
};

#endif // CS_LISTENER_H
