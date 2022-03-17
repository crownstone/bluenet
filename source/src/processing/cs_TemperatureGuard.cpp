/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 8, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_AutoConfig.h>
#include <processing/cs_TemperatureGuard.h>
#include <storage/cs_State.h>

TemperatureGuard::TemperatureGuard() :
		_appTimerId(NULL),
		_maxChipTemp(g_MAX_CHIP_TEMPERATURE),
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
	State::getInstance().get(CS_TYPE::CONFIG_MAX_CHIP_TEMP, &_maxChipTemp, sizeof(_maxChipTemp));

	_dimmerTempInverted = boardConfig.flags.dimmerTempInverted;

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);

	_comp = &COMP::getInstance();
	TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP) pwmTempThresholdUp;
	TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN) pwmTempThresholdDown;
	State::getInstance().get(CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP, &pwmTempThresholdUp, sizeof(pwmTempThresholdUp));
	State::getInstance().get(CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN, &pwmTempThresholdDown, sizeof(pwmTempThresholdDown));
	_comp->init(boardConfig.pinAinDimmerTemp, pwmTempThresholdDown, pwmTempThresholdUp, nullptr);

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
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));

	// Check chip temperature, send event if it changed
	bool chipTempError = getTemperature() > _maxChipTemp ? true : false;
	if (chipTempError) {
		curEvent = CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD;
	}
	else {
		curEvent = CS_TYPE::EVT_CHIP_TEMP_OK;
	}

	// Set state before dispatching event, so that errors are set when handling the event.
	if (chipTempError && !stateErrors.errors.chipTemp) {
		stateErrors.errors.chipTemp = true;
		State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	}

	if (curEvent != _lastChipTempEvent) {
		LOGd("Dispatch event $typeName(%u)", curEvent);
		event_t event(curEvent);
		EventDispatcher::getInstance().dispatch(event);
		_lastChipTempEvent = curEvent;
	}


	// Check PWM temperature, send event if it changed
	bool compVal = _comp->sample();
	bool dimmerTempError;
	if (_dimmerTempInverted) {
		dimmerTempError = compVal ? false : true;
	}
	else {
		dimmerTempError = compVal ? true : false;
	}

	if (dimmerTempError) {
		curEvent = CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD;
	}
	else {
		curEvent = CS_TYPE::EVT_DIMMER_TEMP_OK;
	}

	// Set state before dispatching event, so that errors are set when handling the event.
	if (dimmerTempError && !stateErrors.errors.dimmerTemp) {
		stateErrors.errors.dimmerTemp = true;
		State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	}

	if (curEvent != _lastPwmTempEvent) {
		LOGd("Dispatch event $typeName(%u)", curEvent);
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
