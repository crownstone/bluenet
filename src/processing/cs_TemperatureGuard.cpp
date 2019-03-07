/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 8, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_TemperatureGuard.h"

#include "storage/cs_State.h"

TemperatureGuard::TemperatureGuard() :
		_appTimerId(NULL),
		_maxChipTemp(MAX_CHIP_TEMP),
		_comp(NULL)
	{
		_appTimerData = { {0} };
		_appTimerId = &_appTimerData;
	}

// This callback is decoupled from interrupt
void comp_event_callback(CompEvent_t event) {
	TemperatureGuard::getInstance().handleCompEvent(event);
}

void TemperatureGuard::init(const boards_config_t& boardConfig) {

	State::getInstance().get(CS_TYPE::CONFIG_MAX_CHIP_TEMP, &_maxChipTemp, PersistenceMode::STRATEGY1);

	_pwmTempInverted = boardConfig.flags.pwmTempInverted;

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);

	_comp = &COMP::getInstance();
	float pwmTempThresholdUp;
	float pwmTempThresholdDown;
	State::getInstance().get(CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP, &pwmTempThresholdUp, PersistenceMode::STRATEGY1);
	State::getInstance().get(CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN, &pwmTempThresholdDown, PersistenceMode::STRATEGY1);
	_comp->init(boardConfig.pinAinPwmTemp, pwmTempThresholdDown, pwmTempThresholdUp);
//	_comp->setEventCallback(comp_event_callback);

	_lastChipTempEvent = CS_TYPE::EVT_CHIP_TEMP_OK;
	_lastPwmTempEvent = CS_TYPE::EVT_DIMMER_TEMP_OK;
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
	CS_TYPE curEvent;

	// Get the current state errors
	state_errors_t stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors.asInt, PersistenceMode::STRATEGY1);

	// Check chip temperature, send event if it changed
	uint8_t chipTempError = getTemperature() > _maxChipTemp ? 1 : 0;
	if (chipTempError) {
		curEvent = CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD;
	}
	else {
		curEvent = CS_TYPE::EVT_CHIP_TEMP_OK;
	}

	// Set state before dispatching event, so that errors are set when handling the event.
	if (chipTempError && !stateErrors.errors.chipTemp) {
		//! Set chip temp error
		State::getInstance().set(CS_TYPE::STATE_ERROR_CHIP_TEMP, &chipTempError, sizeof(chipTempError), PersistenceMode::STRATEGY1);
	}

	if (curEvent != _lastChipTempEvent) {
		LOGd("Dispatch event %s", TypeName(curEvent));
		event_t event(curEvent);
		EventDispatcher::getInstance().dispatch(event);
		_lastChipTempEvent = curEvent;
	}


	// Check PWM temperature, send event if it changed
	uint32_t compVal = _comp->sample();
	uint8_t pwmTempError;
	if (_pwmTempInverted) {
		pwmTempError = compVal > 0 ? 0 : 1;
	}
	else {
		pwmTempError = compVal > 0 ? 1 : 0;
	}

	if (pwmTempError) {
		curEvent = CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD;
	}
	else {
		curEvent = CS_TYPE::EVT_DIMMER_TEMP_OK;
	}

	// Set state before dispatching event, so that errors are set when handling the event.
	if (pwmTempError && !stateErrors.errors.pwmTemp) {
		// Set pwm temp error
		st_value_t error;
		error.u8 = pwmTempError;
		State::getInstance().set(CS_TYPE::STATE_ERROR_DIMMER_TEMP, &error, sizeof(error), PersistenceMode::STRATEGY1);
	}

	if (curEvent != _lastPwmTempEvent) {
		LOGd("Dispatch event %s", TypeName(curEvent));
		event_t event(curEvent);
		EventDispatcher::getInstance().dispatch(event);
		_lastPwmTempEvent = curEvent;
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
