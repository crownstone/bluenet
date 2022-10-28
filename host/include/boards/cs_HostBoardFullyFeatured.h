/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 19, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>

/**
 * This board has all pins set to different non-none values, and as many features as possible set to true.
 * Used for debugging.
 */
void asHostFullyFeatured(boards_config_t* config) {
	uint8_t currentPin = 1;
	config->pinDimmer                       = currentPin++;
	config->pinRelayOn                      = currentPin++;
	config->pinRelayOff                     = currentPin++;

	config->pinAinCurrent[GAIN_LOW]         = currentPin++;
	config->pinAinCurrent[GAIN_HIGH]        = currentPin++;

	config->pinAinVoltage[GAIN_LOW]         = currentPin++;
	config->pinAinVoltage[GAIN_HIGH]        = currentPin++;

	config->pinAinZeroRef                   = currentPin++;
	config->pinAinDimmerTemp                = currentPin++;

	config->pinRx                           = currentPin++;
	config->pinTx                           = currentPin++;

	config->pinGpio[0]                      = currentPin++;
	config->pinGpio[1]                      = currentPin++;
	config->pinGpio[2]                      = currentPin++;
	config->pinGpio[3]                      = currentPin++;

	config->pinLed[LED_RED]                 = currentPin++;
	config->pinLed[LED_GREEN]               = currentPin++;

	config->flags.dimmerInverted            = true;
	config->flags.enableUart                = true;
	config->flags.enableLeds                = true;
	config->flags.ledInverted               = false;
	config->flags.dimmerTempInverted        = true;
	config->flags.usesNfcPins               = true;
	config->flags.canTryDimmingOnBoot       = true;
	config->flags.canDimOnWarmBoot          = true;
	config->flags.dimmerOnWhenPinsFloat     = true;

	config->deviceType                      = DEVICE_CROWNSTONE_BUILTIN_ONE;

	// All the values below are just copied from configuration values from other hardware and should be adjusted.
	config->voltageMultiplier[GAIN_SINGLE]  = 0.2f;
	config->currentMultiplier[GAIN_SINGLE]  = 0.004f;
	config->voltageOffset[GAIN_SINGLE]      = 0;

	config->currentOffset[GAIN_LOW]         = -270;
	config->currentOffset[GAIN_HIGH]        = -270;

	config->powerOffsetMilliWatt            = 9000;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageAdcRangeMilliVolt        = 1800;

	// ADC values [-2048, 2047] map to [REF - 0.6V, REF + 0.6V].
	config->currentAdcRangeMilliVolt        = 600;

	config->pwmTempVoltageThreshold         = 0.3639;
	config->pwmTempVoltageThresholdDown     = 0.3135;

	config->minTxPower                      = -20;

	config->scanWindowUs                    = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold = -35;
}
