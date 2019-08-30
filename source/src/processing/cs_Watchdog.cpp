/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 13, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/*
 * Watchdog to keep track of keep alive messages sent from the phone to the crownstone. A keep alive informs the crownstone
 * that the user is around and helps to trigger timeouts when the user leaves the house. I.e. since the user won't be
 * around anymore to inform the crownstone about the action it should take when he leaves the house, we put the action
 * in a keep alive message, like a heartbeat. As long as the crownstone receives keep alive messages, everything is fine.
 * Once no more keep alive messages are received and the timeout expires, the crownstone will execute the action defined
 * in the last keep alive.
 *
 * To achieve this, we do the following:
 *   1. When receiving a keep alive message, a timer is started with timeout equal to the timeout defined in the keep
 *      alive message plus half the keep alive interval.
 *   2. The keep alive message is stored to retrieve when the timeout expires.
 *   3. Every time a new keep alive message is received, the timer is reset to the timeout in the new keep alive
 *      message
 *   4. If the timeout expires, i.e. no keep alive messages were received, the action from the last stored keep alive
 *      message is triggered
 *
 * Note: If a keep alive without state is received, the timeout and action of the last keep alive state message is
 *       used.
 *
 * Note: If by unfortunate happenstance (crownstone is reset) no last keep alive state message is found, the keep
 *       alive will be ignored and nothing happens
 *
 * The keep alive interval can be adjusted through the Configuration characteristic, see CONFIG_KEEP_ALIVE_INTERVAL
 */

#include <processing/cs_Watchdog.h>
#include <storage/cs_State.h>
#include <drivers/cs_RTC.h>
#include <processing/cs_CommandHandler.h>
#include <drivers/cs_Timer.h>

#define LOGWatchdogDebug LOGnone

void keep_alive_timeout(void* p_context) {
	Watchdog::getInstance().keepAliveTimeout();
}

Watchdog::Watchdog() :
	_hasKeepAliveState(false),
	_keepAliveTimerId(NULL)
{
	_keepAliveTimerData = { {0} };
	_keepAliveTimerId = &_keepAliveTimerData;

	EventDispatcher::getInstance().addListener(this);
};

void Watchdog::init() {
	Timer::getInstance().createSingleShot(_keepAliveTimerId, keep_alive_timeout);
}

void Watchdog::keepAlive() {
	if (_hasKeepAliveState) {
		uint32_t timeout = _lastKeepAlive.timeout * 1000;
		LOGWatchdogDebug("Reset keep alive, timeout in %d ms", timeout);
		Timer::getInstance().reset(_keepAliveTimerId, MS_TO_TICKS(timeout), NULL);
	}
	else {
		LOGWatchdogDebug("No keep alive state found!!");
	}
}

void Watchdog::keepAliveTimeout() {
	LOGi("Keep alive timeout");
	if (_hasKeepAliveState) {
		if (_lastKeepAlive.action != NO_CHANGE) {
			Switch::getInstance().setSwitch(_lastKeepAlive.switchCmd);
		}
		else {
			LOGi("No change");
		}
	}
	else {
		LOGw("No keep alive state found!!");
	}
}

void Watchdog::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::EVT_KEEP_ALIVE_STATE: {
			_lastKeepAlive = *(TYPIFY(EVT_KEEP_ALIVE_STATE)*)event.data;
			LOGWatchdogDebug("action=%u switch=%u timeout=%u", _lastKeepAlive.action, _lastKeepAlive.switchCmd, _lastKeepAlive.timeout);
			_hasKeepAliveState = true;
			keepAlive();
			break;
		}
		case CS_TYPE::EVT_KEEP_ALIVE: {
			keepAlive();
			break;
		}
		default: {}
	}
}
