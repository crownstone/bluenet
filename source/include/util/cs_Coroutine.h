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
#include <events/cs_Event.h>

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
public:
	enum class Mode : uint8_t {
		Repeat,
		Single,
		Paused,
		StartedSingle,
		StartedRepeat,
	};

	typedef std::function<uint32_t(void)> Action;

	/**
	 * A function takes no parameters andreturns the amount of ticks before it should be called again.
	 *
	 * Note: You can use `return Coroutine::delayS(4);` to convert from seconds to ticks.
	 */
	Action _action;

	/**
	 * By default coroutines will start repeating their action immediately.
	 *
	 * To setup a coroutine for single-shot, construct it with mode Paused and call startSingleS(delay).
	 */
	Coroutine(Action action, Mode mode = Mode::Repeat) : _action(action), _mode(mode) {}
	Coroutine() = default;

	// ---------------------------------------------------------------

	/**
	 * Handles all administration of Coroutine and calls _action at the configured time(s).
	 *
	 * Return true if given evt was a EVT_TICK event. False otherwise.
	 *
	 * Call this in your eventhandlers handleEvent method.
	 */
	bool handleEvent(event_t& evt);

	// ---------------------------------------------------------------

	/**
	 * Schedules the coroutine action to run after given amount of miliseconds.
	 * Sets the coroutine mode to paused afterwards.
	 */
	void startSingleMs(uint32_t ms = 0);

	/**
	 * Schedules the coroutine action to run after given amount of seconds.
	 * Sets the coroutine mode to paused afterwards.
	 */
	void startSingleS(uint32_t s = 0);

	/**
	 * Schedules the coroutine action to start running after given amount of miliseconds.
	 * Note: even with delay set to 0, the _action will wait for the next tick event.
	 */
	void startRepeatMs(uint32_t ms = 0);

	/**
	 * Schedules the coroutine action to start running after given amount of seconds.
	 * Note: even with delay set to 0, the _action will wait for the next tick event.
	 */
	void startRepeatS(uint32_t s = 0);

	/**
	 * cancel pending call to action.
	 */
	void pause();

	// ---------------------------------------------------------------

	// conversion utilities.

	static uint32_t delayMs(uint32_t ms) {
		return ms / TICK_INTERVAL_MS;
	}

	static uint32_t delayS(uint32_t s){
		return delayMs(s * 1000);
	}

private:
	uint32_t _nextCallTickcount = 0;

	Mode _mode = Mode::Repeat;

	/**
	 * true if currentTickCount is beyond _nextCallTickcount and _action is non-empty.
	 */
	bool shouldRunAction(uint32_t currentTickCount);

	/**
	 * Checks the current mode of operation, call the action if necessary and update
	 * administration variables.
	 *
	 * TODO: not doing roll-over checks here yet..
	 */
	void onTick(uint32_t currentTickCount);
};
