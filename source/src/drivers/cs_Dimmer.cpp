/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_Dimmer.h>
#include <drivers/cs_PWM.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>
#include <util/cs_Error.h>

#include <test/cs_Test.h>

void Dimmer::init(const boards_config_t& board) {
	if(_initialized) {
		LOGw("Dimmer init was already executed.");
		return;
	}

	_hardwareBoard = board.hardwareBoard;
	_pinEnableDimmer = board.pinEnableDimmer;
	_hasDimmer = board.pinDimmer != PIN_NONE;

	if (!_hasDimmer) {
		return;
	}

	if (_pinEnableDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(_pinEnableDimmer);
		nrf_gpio_pin_clear(_pinEnableDimmer);
	}

	TYPIFY(CONFIG_PWM_PERIOD) pwmPeriodUs;
	State::getInstance().get(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriodUs, sizeof(pwmPeriodUs));

	State::getInstance().get(CS_TYPE::STATE_SOFT_ON_SPEED, &_softOnSpeed, sizeof(_softOnSpeed));

	LOGd("init enablePin=%u dimmerPin=%u inverted=%u period=%u µs softOnSpeed=%u", board.pinEnableDimmer, board.pinDimmer, board.flags.dimmerInverted, pwmPeriodUs, _softOnSpeed);

	pwm_config_t pwmConfig;
	pwmConfig.channelCount = 1;
	pwmConfig.period_us = pwmPeriodUs;
	pwmConfig.channels[0].pin = board.pinDimmer;
	pwmConfig.channels[0].inverted = board.flags.dimmerInverted;

	PWM::getInstance().init(pwmConfig);

	_initialized = true;
}

bool Dimmer::hasDimmer() {
	return _hasDimmer;
}

void Dimmer::start() {
	if (!_hasDimmer) {
		return;
	}
	if (_started) {
		return;
	}
	LOGd("start");
	assert(_initialized == true, "Not initialized");

	enable();

	TYPIFY(CONFIG_START_DIMMER_ON_ZERO_CROSSING) startDimmerOnZeroCrossing;
	State::getInstance().get(CS_TYPE::CONFIG_START_DIMMER_ON_ZERO_CROSSING, &startDimmerOnZeroCrossing, sizeof(startDimmerOnZeroCrossing));

	switch (_hardwareBoard) {
		case PCA10036:
		case PCA10040:
		case PCA10100:
		case PCA10056:
			// These dev boards don't have power measurement, so no zero crossing.
			PWM::getInstance().start(false);
			break;
		default:
			PWM::getInstance().start(startDimmerOnZeroCrossing);
			break;
	}

	_started = true;
}

bool Dimmer::set(uint8_t intensity, bool fade) {
	if (!_hasDimmer) {
		return false;
	}
	LOGd("set %u fade=%u", intensity, fade);
	assert(_initialized == true, "Not initialized");
	if (!_enabled && intensity > 0) {
		LOGd("Dimmer not enabled");
		return false;
	}

	uint8_t speed = fade ? _softOnSpeed : 100;

	TEST_PUSH_EXPR_D(this, "intensity", intensity);
	PWM::getInstance().setValue(0, intensity, speed);
	
	return true;
}

void Dimmer::setSoftOnSpeed(uint8_t speed) {
	if (!_hasDimmer) {
		return;
	}
	LOGd("setSoftOnSpeed %u", speed);
	_softOnSpeed = speed;
}

void Dimmer::enable() {
	if (!_hasDimmer) {
		return;
	}
	if (_pinEnableDimmer != PIN_NONE) {
		LOGd("enable");
		nrf_gpio_pin_set(_pinEnableDimmer);
		_enabled = true;
	}
}
