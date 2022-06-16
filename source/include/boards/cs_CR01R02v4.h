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
 * Dev board
 * *******************************************************************************************************************/

/**
 * This is a smart outlet.
 *
 *   + There is no dimmer.
 *   + There is no temperature measured near the dimmer.
 *   + There is zero-crossing detect for both voltage and current.
 *   + There is a shutter as binary input.
 *   + There is an RGB led, R pinLed[0], G pinLed[1], B pinLed[2].
 *   + There is a touch input. This also needs to be enabled.
 *   + There is NOR flash available.
 *
 * We are still in a testing phase.
 *
 *   + The first GPIO pin is set to the line that has to be enabled for the touch sensor.
 *   + The second button is set to the touch sensor
 *
 * It's easy to test with a microapp. That doesn't require changing code in the firmware itself.
 * In a microapp first set pinGpio[0] or (P1.00) to true and start listening to BUTTON2 (P0.08).
 *
 * The shutter is placed on pinGpio[1].
 *
 * The zero-crossings are placed on gpio 2 and 3 (should be configured as inputs). Note that this is propagated through
 * the firmware towards the microapp. That won't be fast enough to do something real-time with it.
 */
void asCR01R02v4(boards_config_t* config) {
	config->pinDimmer                       = PIN_NONE;
	config->pinRelayOn                      = 12;
	config->pinRelayOff                     = 11;
	config->pinAinCurrent[GAIN_SINGLE]      = GpioToAin(29);
	config->pinAinVoltage[GAIN_SINGLE]      = GpioToAin(30); // 31
	// Will become available as ref=ain0 (config.referencePin)
	config->pinAinZeroRef                   = PIN_NONE;
	// config->pinAinDimmerTemp                = PIN_NONE;
	config->pinAinDimmerTemp                = GpioToAin(3);

	config->pinRx                           = PIN_NONE;
	config->pinTx                           = 28;
	
	config->pinGpio[0]                      = 17;
	config->pinGpio[1]                      = 20;
	config->pinGpio[2]                      = 13;
	config->pinGpio[3]                      = 14;
	config->pinGpio[4]                      = 6;
	config->pinGpio[5]                      = 7;

	config->pinButton[0]                    = PIN_NONE;
	config->pinButton[1]                    = 8;
	config->pinButton[2]                    = 24;
	config->pinButton[3]                    = 25;

	// The fourth LED is used to indicate the state of the dimmer
	config->pinLed[0]                       = PIN_NONE;
	config->pinLed[1]                       = PIN_NONE;
	config->pinLed[2]                       = PIN_NONE;
	config->pinLed[3]                       = PIN_NONE;

	config->flags.dimmerInverted            = false;
	config->flags.enableUart                = true;
	config->flags.enableLeds                = false;
	config->flags.ledInverted               = false;
	config->flags.dimmerTempInverted        = false;
	config->flags.usesNfcPins               = false;

	config->deviceType                      = DEVICE_CROWNSTONE_BUILTIN_ONE;

	// All values below are set to something rather than nothing, but are not truly in use.
	config->voltageOffset[GAIN_SINGLE]      = 0;
	config->currentOffset[GAIN_LOW]         = 0;
	config->currentOffset[GAIN_HIGH]        = 0;
	config->powerOffsetMilliWatt            = 0;

	// ADC values [0, 4095] map to [0, 3.6V].
	config->voltageAdcRangeMilliVolt        = 3600;
	config->currentAdcRangeMilliVolt        = 3600;

	config->pwmTempVoltageThreshold         = 3.6;
	config->pwmTempVoltageThresholdDown     = 1.0;

	config->minTxPower                      = -40;

	config->scanWindowUs                    = 3 * config->scanIntervalUs / 4;
	config->tapToToggleDefaultRssiThreshold = -35;
}
