/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>

/* ********************************************************************************************************************
 * Crownstone Built-in Zero
 * *******************************************************************************************************************/

/**
 * Most important to know about this board is that it cannot provide enough power for 100% scanning.
 *
 * This Crownstone hardware (a built-in) has a voltage range of 1200 mV (it will be configured with a gain of 1/2).
 * It is either possible to use VDD as reference (VDD/4) or an internal reference (0.6V). It is configured with an
 * internal reference.
 */
void asACR01B1D(boards_config_t* config) {
	config->pinDimmer                       = 8;
	config->pinRelayOn                      = 6;
	config->pinRelayOff                     = 7;
	config->pinAinCurrent[GAIN_SINGLE]      = GpioToAin(4);
	config->pinAinVoltage[GAIN_SINGLE]      = GpioToAin(3);
	config->pinAinDimmerTemp                = GpioToAin(5);

	config->pinRx                           = 20;
	config->pinTx                           = 19;
	config->pinLed[LED_RED]                 = 10;
	config->pinLed[LED_GREEN]               = 9;

	config->flags.dimmerInverted            = false;
	config->flags.enableUart                = false;
	config->flags.enableLeds                = false;
	config->flags.ledInverted               = false;
	config->flags.dimmerTempInverted        = false;
	config->flags.usesNfcPins               = false;  // Set to true if you want to use the LEDs.
	config->flags.canTryDimmingOnBoot       = false;
	config->flags.canDimOnWarmBoot          = false;
	config->flags.dimmerOnWhenPinsFloat     = true;

	config->deviceType                      = DEVICE_CROWNSTONE_BUILTIN;

	// TODO: Explain this value
	config->voltageMultiplier[GAIN_SINGLE]  = 0.2f;

	// TODO: Explain this value
	config->currentMultiplier[GAIN_SINGLE]  = 0.0044f;

	// TODO: Explain this value
	config->voltageOffset[GAIN_SINGLE]      = 1993;

	// TODO: Explain this value
	config->currentOffset[GAIN_SINGLE]      = 1980;

	// TODO: Explain this value
	config->powerOffsetMilliWatt            = 3500;

	// ADC values [0, 4095] map to [0V, 1.2V].
	config->voltageAdcRangeMilliVolt        = 1200;
	config->currentAdcRangeMilliVolt        = 1200;

	// TODO: Explain these comments. About 1.5kOhm --> 90-100C
	config->pwmTempVoltageThreshold         = 0.76;
	// TODO: Explain these comments. About 0.7kOhm --> 70-95C
	config->pwmTempVoltageThresholdDown     = 0.41;

	config->minTxPower                      = -20;

	// This board cannot provide enough power for 100% scanning!
	// So set a scan window of 75% of the interval.
	config->scanWindowUs                    = 3 * config->scanIntervalUs / 4;
	config->tapToToggleDefaultRssiThreshold = -35;
}
