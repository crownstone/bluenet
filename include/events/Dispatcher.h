/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)                                                                       
 * Date: 1 Dec., 2014                                                                                                   
 * License: LGPLv3+, Apache License, or MIT, your choice                                                                
 */

#ifndef CS_DISPATCHER_H
#define CS_DISPATCHER_H

#include <vector>
#include <events/Listener.h>

class Dispatcher {
public:
	// add a listener
	void addListener(Listener *listener);

	// remove one
	void removeListener(Listener *listener);
protected:
	void dispatch();
private:
	std::vector<Listener*> _listeners;
};

#endif // CS_DISPATCHER_H

