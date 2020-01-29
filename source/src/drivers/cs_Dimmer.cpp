/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_Dimmer.h>
#include <drivers/cs_PWM.h>
#include <storage/cs_State.h>

Dimmer::Dimmer(const boards_config_t& board, uint32_t pwmPeriodUs, bool startDimmerOnZeroCrossing) :
	hardwareBoard(board.hardwareBoard)
{
	TYPIFY(CONFIG_PWM_PERIOD) pwmPeriod;
	State::getInstance().get(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriod, sizeof(pwmPeriod));

	pwm_config_t pwmConfig;
	pwmConfig.channelCount = 1;
	pwmConfig.period_us = pwmPeriodUs;
	pwmConfig.channels[0].pin = board.pinGpioPwm;
	pwmConfig.channels[0].inverted = board.flags.pwmInverted;

	PWM::getInstance().init(pwmConfig);
}

void Dimmer::start() {
	if (started) {
		return;
	}
	started = true;

	enable();

	TYPIFY(CONFIG_START_DIMMER_ON_ZERO_CROSSING) startDimmerOnZeroCrossing;
	State::getInstance().get(CS_TYPE::CONFIG_START_DIMMER_ON_ZERO_CROSSING, &startDimmerOnZeroCrossing, sizeof(startDimmerOnZeroCrossing));

	switch (hardwareBoard) {
		case PCA10036:
		case PCA10040:
			// These dev boards don't have power measurement, so no zero crossing.
			PWM::getInstance().start(false);
			break;
		default:
			PWM::getInstance().start(startDimmerOnZeroCrossing);
			break;
	}
}

bool Dimmer::set(uint8_t intensity) {
	if (!enabled && intensity > 0) {
		return false;
	}
	PWM::getInstance().setValue(0, intensity);
	return true;
}

void Dimmer::enable() {
	switch (hardwareBoard) {
		// Dev boards
		case PCA10036:
		case PCA10040:
		// Builtin zero
		case ACR01B1A:
		case ACR01B1B:
		case ACR01B1C:
		case ACR01B1D:
		case ACR01B1E:
		// First builtin one
		case ACR01B10B:
		// Plugs
		case ACR01B2A:
		case ACR01B2B:
		case ACR01B2C:
		case ACR01B2E:
		case ACR01B2G:
		// These don't have a dimmer.
		case GUIDESTONE:
		case CS_USB_DONGLE: {
			break;
		}
		// Newer ones have a dimmer enable pin.
		case ACR01B10C:
		default: {
			nrf_gpio_pin_set(_pinEnableDimmer);
			break;
		}
	}
	enabled = true;
}
