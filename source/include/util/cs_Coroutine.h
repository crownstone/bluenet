/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <functional>

// coroutines
// note: tickrate currently is 10 ticks/s

/**
 * A coroutine essentially is a throttling mechanism: it takes
 * in a tick-event or tick count and executes its action when the
 * time (number of ticks) passed between the last call and the
 * current one is larger than the time the action reported the next
 * call needed to wait.
 *
 * Example:
 *  uint32_t sayHi() { LOGd("hi"); return 42; }
 *	Coroutine hiSayer (sayHi);
 *
 *	for (int i = 0; ; i++){
 *		hiSayer(i);
 *	}
 *
 * This will log "hi" only one in 42 calls.
 *
 * Note that the return value of sayHi determines the delay, so that
 * a coroutine can dynamically determine if it needs to be called more
 * often or not.
 *
 */
class Coroutine{
private:
	uint32_t next_call_tickcount = 0;
public:
	typedef std::function<uint32_t(void)> Action;

	// void function that returns the amount of ticks before it should be called again.
	Action action;

	Coroutine() = default;
	Coroutine(Action a) : action(a) {}

	void operator()(uint32_t current_tick_count){
		// not doing roll-over checks here yet..
		if(current_tick_count > next_call_tickcount && action){
			auto ticks_to_wait = action();
			next_call_tickcount = current_tick_count + ticks_to_wait;
		}
	}

	/**
	 * utility wrapper to simply pass an event.
	 * Returns true if the event type was tick.
	 */
	bool operator()(event_t& evt){
		if(evt.type == CS_TYPE::EVT_TICK){
			// forward call to uint32_t
			(*this)(*reinterpret_cast<uint32_t*>(evt.data));
			return true;
		}
		return false;
	}
};
