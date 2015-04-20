/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)                                                                       
 * Date: 1 Dec., 2014                                                                                                   
 * License: LGPLv3+, Apache License, or MIT, your choice                                                                
 */

#include <algorithm>
#include "events/cs_Dispatcher.h"

void Dispatcher::addListener(Listener *listener) {
	for (int i = 0; i < MAX_LISTENERS; i++) {
		if (_listeners[i] == NULL) {
			_listeners[i] = listener;
		}
	}
//	_listeners.push_back(listener);
}

void Dispatcher::removeListener(Listener *listener) {
	for (int i = 0; i < MAX_LISTENERS; i++) {
		if (_listeners[i] == listener) {
			_listeners[i] = NULL;
		}
	}
	
/*	_listeners.erase(
		std::remove( std::begin(_listeners), std::end(_listeners), listener),
		std::end(_listeners)
	);*/
}

void Dispatcher::dispatch() {
	for (int i = 0; i < MAX_LISTENERS; i++) {
		if (_listeners[i] != NULL) {
			_listeners[i]->handleEvent();
		}
	}
	/*
	for (auto listener : _listeners) {
		listener->handleEvent();
	}*/
}
