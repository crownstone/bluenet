/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)                                                                       
 * Date: 1 Dec., 2014                                                                                                   
 * License: LGPLv3+, Apache License, or MIT, your choice                                                                
 */

#include <events/Dispatcher.h>
#include <algorithm>

void Dispatcher::addListener(Listener *listener) {
	_listeners.push_back(listener);
}

void Dispatcher::removeListener(Listener *listener) {
	_listeners.erase(
		std::remove( std::begin(_listeners), std::end(_listeners), listener),
		std::end(_listeners)
	);
}

void Dispatcher::dispatch() {
	for (auto listener : _listeners) {
		listener->handleEvent();
	}
}

