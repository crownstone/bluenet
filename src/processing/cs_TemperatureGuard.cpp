/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 8, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "processing/cs_TemperatureGuard.h"

#include "storage/cs_State.h"

TemperatureGuard::TemperatureGuard() :
		EventListener(CONFIG_MAX_CHIP_TEMP),
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerId(NULL),
#else
		_appTimerId(UINT32_MAX),
#endif
		_maxChipTemp(MAX_CHIP_TEMP),
		_comp(NULL)
	{
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerData = { {0} };
		_appTimerId = &_appTimerData;
#endif
	}

// This callback is decoupled from interrupt
void comp_event_callback(CompEvent_t event) {
	TemperatureGuard::getInstance().handleCompEvent(event);
}

void TemperatureGuard::init(boards_config_t* boardConfig) {
	EventDispatcher::getInstance().addListener(this);

	Settings::getInstance().get(CONFIG_MAX_CHIP_TEMP, &_maxChipTemp);

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);

	_comp = &COMP::getInstance();
	_comp->init(boardConfig->pinAinPwmTemp, boardConfig->pwmTempVoltageThresholdDown, boardConfig->pwmTempVoltageThreshold);
//	_comp->setEventCallback(comp_event_callback);

	_lastChipTempEvent = EVT_CHIP_TEMP_OK;
	_lastPwmTempEvent = EVT_PWM_TEMP_OK;
}


void TemperatureGuard::handleCompEvent(CompEvent_t event) {
	switch (event) {
	case COMP_EVENT_DOWN:
		LOGd("down");
		break;
	case COMP_EVENT_UP:
		LOGd("up");
		break;
	case COMP_EVENT_CROSS:
		LOGd("cross");
		break;
	default:
		break;
	}
}

void TemperatureGuard::tick() {
	GeneralEventType curEvent;

	// Check chip temperature, send event if it changed
	uint8_t chipTempError = getTemperature() > _maxChipTemp ? 1 : 0;
	if (chipTempError) {
		curEvent = EVT_CHIP_TEMP_ABOVE_THRESHOLD;
	}
	else {
		curEvent = EVT_CHIP_TEMP_OK;
	}
	if (curEvent != _lastChipTempEvent) {
		LOGd("Dispatch event %d", curEvent);
		EventDispatcher::getInstance().dispatch(curEvent);
		_lastChipTempEvent = curEvent;
	}

	// Check PWM temperature, send event if it changed
	uint32_t compVal = _comp->sample();
	uint8_t pwmTempError = compVal > 0 ? 1 : 0;
	if (pwmTempError) {
		curEvent = EVT_PWM_TEMP_ABOVE_THRESHOLD;
	}
	else {
		curEvent = EVT_PWM_TEMP_OK;
	}
	if (curEvent != _lastPwmTempEvent) {
		LOGd("Dispatch event %d", curEvent);
		EventDispatcher::getInstance().dispatch(curEvent);
		_lastPwmTempEvent = curEvent;
	}

	//! Get the current state errors
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, &stateErrors, sizeof(state_errors_t));

	if (pwmTempError && !stateErrors.errors.pwmTemp) {
		//! Set pwm temp error
		State::getInstance().set(STATE_ERROR_PWM_TEMP, pwmTempError);
	}

	if (chipTempError && !stateErrors.errors.chipTemp) {
		//! Set chip temp error
		State::getInstance().set(STATE_ERROR_CHIP_TEMP, chipTempError);
	}

	scheduleNextTick();
}


void TemperatureGuard::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
}

void TemperatureGuard::start() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
	_comp->start(COMP_EVENT_BOTH);
}

void TemperatureGuard::stop() {
	Timer::getInstance().stop(_appTimerId);
}

void TemperatureGuard::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case CONFIG_MAX_CHIP_TEMP:
		_maxChipTemp = *(int32_t*)p_data;
		break;
	}
}
