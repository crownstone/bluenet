/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

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
	uint32_t next_call_tickcount = 0;
public:
	typedef std::function<uint32_t(void)> Action;
	Action action;

	Coroutine(Action a) : action(a) {}

	void operator()(uint32_t current_tick_count){
		// not doing roll-over checks here yet..
		if(current_tick_count > next_call_tickcount){
			auto ticks_to_wait = action();
			next_call_tickcount = current_tick_count + ticks_to_wait;
		}
	}
};
