/*
 * CurrentLimit.h
 *
 *  Created on: Dec 2, 2014
 *      Author: Bart van Vliet
 */

#ifndef __CURRENTLIMIT__H__
#define __CURRENTLIMIT__H__

#include <events/Listener.h>
#include <drivers/LPComp.h>

class CurrentLimit: public Listener {
public:
	CurrentLimit();

	~CurrentLimit();

	void init();

	// We get a dispatch from the LP comparator
	void handleEvent();
	void handleEvent(uint8_t type);
};



#endif /* __CURRENTLIMIT__H__ */
