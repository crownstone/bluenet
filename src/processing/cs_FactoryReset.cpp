/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_FactoryReset.h"
#include "cfg/cs_Config.h"
#include "storage/cs_State.h"
#include "drivers/cs_RTC.h"
#include "ble/cs_Stack.h"
#include "processing/cs_CommandHandler.h"
#include "processing/cs_Switch.h"

FactoryReset::FactoryReset() : _recoveryEnabled(true), _rtcStartTime(0),
#if (NORDIC_SDK_VERSION >= 11)
		_recoveryDisableTimerId(NULL),
		_recoveryProcessTimerId(NULL)
#else
		_recoveryDisableTimerId(UINT32_MAX)
#endif
{
#if (NORDIC_SDK_VERSION >= 11)
	_recoveryDisableTimerData = { {0} };
	_recoveryDisableTimerId = &_recoveryDisableTimerData;

	_recoveryProcessTimerData = { {0} };
	_recoveryProcessTimerId = &_recoveryProcessTimerData;
#endif
}

void FactoryReset::init() {
	Timer::getInstance().createSingleShot(_recoveryDisableTimerId, (app_timer_timeout_handler_t)FactoryReset::staticTimeout);
	Timer::getInstance().createSingleShot(_recoveryProcessTimerId, (app_timer_timeout_handler_t)FactoryReset::staticProcess);
	resetTimeout();
}

void FactoryReset::resetTimeout() {
	_recoveryEnabled = true;
	_rtcStartTime = RTC::getCount();
	//! stop the currently running timer, this does not cause problems if the timer has not been set yet.
	Timer::getInstance().stop(_recoveryDisableTimerId);
	Timer::getInstance().start(_recoveryDisableTimerId, MS_TO_TICKS(FACTORY_RESET_TIMEOUT), this);
}

void FactoryReset::timeout() {
	_recoveryEnabled = false;
	LOGi("recovery period expired.")
	uint8_t resetState;
	State::getInstance().get(STATE_FACTORY_RESET, resetState);
	if (resetState == FACTORY_RESET_STATE_LOWTX) {
		LOGi("change to  normal")
		Stack::getInstance().changeToNormalTxPowerMode();
	}
	State::getInstance().set(STATE_FACTORY_RESET, (uint8_t)FACTORY_RESET_STATE_NORMAL);
}

/**
 * This has been put in a timer because we need to read the result after writing to see if the process is successful.
 * Without the timer (and without logging) the disconenct is too fast for us to read the result.
 */
void FactoryReset::process() {
	uint8_t resetState;
	State::getInstance().get(STATE_FACTORY_RESET, resetState);
	if (resetState == FACTORY_RESET_STATE_NORMAL) {
		State::getInstance().set(STATE_FACTORY_RESET, (uint8_t) FACTORY_RESET_STATE_LOWTX);
		LOGd("recovery: go to low tx");
		Stack::getInstance().changeToLowTxPowerMode();
		Stack::getInstance().disconnect();
		resetTimeout();
	}
}

void FactoryReset::enableRecovery(bool enable) {
	LOGd("recovery: %i", enable);
	_recoveryEnabled = enable;
}

bool FactoryReset::recover(uint32_t resetCode) {
	//! we check the RTC in case the Timer is busy.
	if (!_recoveryEnabled || RTC::difference(RTC::getCount(), _rtcStartTime) > RTC::msToTicks(FACTORY_RESET_TIMEOUT)) {
		LOGi("recovery off")
		return false;
	}

	if (!validateResetCode(resetCode)) {
		LOGi("bad recovery code")
		return false;
	}

	uint8_t resetState;
	State::getInstance().get(STATE_FACTORY_RESET, resetState);
	switch (resetState) {
	case FACTORY_RESET_STATE_NORMAL:
		//! just in case, we stop the timer so we cannot flood this mechanism.
		Timer::getInstance().stop(_recoveryProcessTimerId);
		Timer::getInstance().start(_recoveryProcessTimerId, MS_TO_TICKS(FACTORY_PROCESS_TIMEOUT), this);
		return true;
//		break;
	case FACTORY_RESET_STATE_LOWTX:
		State::getInstance().set(STATE_FACTORY_RESET, (uint8_t)FACTORY_RESET_STATE_RESET);
		LOGd("recovery: factory reset");

		// the reset delayed in here should be sufficient
		return performFactoryReset();
//		break;
	case FACTORY_RESET_STATE_RESET:
		// What to do here?
		break;
	default:
		break;
	}
	return false;
}

bool FactoryReset::validateResetCode(uint32_t resetCode) {
	return resetCode == FACTORY_RESET_CODE;
}

bool FactoryReset::factoryReset(uint32_t resetCode) {
	if (validateResetCode(resetCode)) {
		return performFactoryReset();
	}
	return false;
}

bool FactoryReset::performFactoryReset() {
	LOGf("factory reset");

	//! Go into factory reset mode after next reset.
	State::getInstance().set(STATE_OPERATION_MODE, (uint8_t)OPERATION_MODE_FACTORY_RESET);

	LOGi("Going into factory reset mode, rebooting device in 2s ...");
	CommandHandler::getInstance().resetDelayed(GPREGRET_SOFT_RESET);

	return true;
}

bool FactoryReset::finishFactoryReset(uint8_t deviceType) {
	if (IS_CROWNSTONE(deviceType)) {
		// Set switch to initial value: off
		Switch::getInstance().setSwitch(0);
	}

	// First clear sensitive data: keys
	Settings::getInstance().factoryReset(FACTORY_RESET_CODE);

	// Clear other data
	State::getInstance().factoryReset(FACTORY_RESET_CODE);

	// Remove bonded devices
	Stack::getInstance().device_manager_init(true);

	// Lastly, go into setup mode after next reset
	State::getInstance().set(STATE_OPERATION_MODE, (uint8_t)OPERATION_MODE_SETUP);

	LOGi("Factory reset done, rebooting device in 2s ...");
	CommandHandler::getInstance().resetDelayed(GPREGRET_SOFT_RESET);
	return true;
}
