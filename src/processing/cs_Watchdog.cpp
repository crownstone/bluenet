/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 13, 2016
 * License: LGPLv3+
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
#include <storage/cs_Settings.h>
#include <drivers/cs_RTC.h>
#include <processing/cs_CommandHandler.h>
#include <drivers/cs_Timer.h>

void keep_alive_timeout(void* p_context) {
	Watchdog::getInstance().keepAliveTimeout();
}

Watchdog::Watchdog() :
	_hasKeepAliveState(false),
//	_lastKeepAliveTimestamp(0),
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
		LOGd("Reset keep alive");
//		_lastKeepAliveTimestamp = RTC::now();
		uint32_t timeout = _lastKeepAlive.timeout * 1000;
//		LOGi("timeout in %d ms", timeout);
		Timer::getInstance().reset(_keepAliveTimerId, MS_TO_TICKS(timeout), NULL);
	} else {
		LOGw("No keep alive state found!!");
	}
}

void Watchdog::keepAliveTimeout() {
	LOGi("Keep alive timeout");

	if (_hasKeepAliveState) {
		if (_lastKeepAlive.action != NO_CHANGE) {
			Switch::getInstance().setSwitch(_lastKeepAlive.switchState.switchState);
		} else {
			LOGi("No change");
		}
	} else {
		LOGw("No keep alive state found!!");
	}
}

void Watchdog::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_KEEP_ALIVE: {
		if (length != 0 && length == sizeof(keep_alive_state_message_payload_t)) {
			_lastKeepAlive = *(keep_alive_state_message_payload_t*)p_data;
			_hasKeepAliveState = true;
		}
		keepAlive();
		break;
	}
	}
}
