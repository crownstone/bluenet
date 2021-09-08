/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <functional>

#include <drivers/cs_Timer.h>
#include <drivers/cs_RTC.h>

// coroutines
// note: coroutine is currently built upon the event buss tickrate
// which is set to about 10 ticks/s. (wishlist: use SystemTime instead.)

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
 *		hiSayer.onTick(i);
 *	}
 *
 * This will log "hi" only one in 42 calls.
 *
 * Note that the return value of sayHi determines the delay, so that
 * a coroutine can dynamically determine if it needs to be called more
 * often or not.
 *
 */
class Coroutine {
private:
	uint32_t nextCallTickcount = 0;

public:
	typedef std::function<uint32_t(void)> Action;

	// void function that returns the amount of tick events before it should be called again.
	Action action;

	Coroutine() = default;
	Coroutine(Action a) : action(a) {}

	/**
	 * To be called on tick event.
	 */
	void onTick(uint32_t currentTickCount) {
		// TODO: not doing roll-over checks here yet..
		if (currentTickCount >= nextCallTickcount && action) {
			auto ticksToWait = action();
			nextCallTickcount = currentTickCount + ticksToWait;
		}
	}

	/**
	 * Convenience function replacing onTick().
	 *
	 * To be called on event.
	 */
	bool handleEvent(event_t& evt) {
		if (evt.type == CS_TYPE::EVT_TICK) {
			this->onTick(*reinterpret_cast<TYPIFY(EVT_TICK)*>(evt.data));
			return true;
		}
		return false;
	}

	uint32_t getNextCallTickCount() const {
		return nextCallTickcount;
	}

	static uint32_t delayMs(uint32_t ms) {
		return ms / TICK_INTERVAL_MS;
	}

	static uint32_t delayS(uint32_t s){
		return delayMs(s * 1000);
	}
};
