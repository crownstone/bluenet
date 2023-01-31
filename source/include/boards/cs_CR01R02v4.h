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
	config->pinAinCurrent[GAIN_LOW]         = GpioToAin(2);
	config->pinAinCurrent[GAIN_HIGH]        = GpioToAin(3);
	config->pinAinVoltage[GAIN_SINGLE]      = GpioToAin(30);
	config->pinAinZeroRef                   = GpioToAin(5);
	config->pinAinEarth                     = GpioToAin(29);
	// config->pinAinDimmerTemp                = PIN_NONE;
	config->pinAinDimmerTemp                = GpioToAin(4);

	config->pinRx                           = 9;
	config->pinTx                           = 10;

	config->pinGpio[0]                      = GetGpioPin(1, 0);
	config->pinGpio[1]                      = GetGpioPin(1, 15);
	config->pinGpio[2]                      = GetGpioPin(1, 8);
	config->pinGpio[3]                      = GetGpioPin(1, 9);

	config->pinButton[0]                    = PIN_NONE;
	config->pinButton[1]                    = 8;
	config->pinButton[2]                    = 24;
	config->pinButton[3]                    = 25;

	config->pinLed[0]                       = 13;
	config->pinLed[1]                       = 14;
	config->pinLed[2]                       = 17;
	config->pinLed[3]                       = PIN_NONE;

	config->pinFlash.cs                     = 19;
	config->pinFlash.clk                    = 20;
	config->pinFlash.dio[0]                 = 21;
	config->pinFlash.dio[1]                 = 22;
	config->pinFlash.dio[2]                 = 23;
	config->pinFlash.dio[3]                 = 24;

	config->flags.dimmerInverted            = false;
	config->flags.enableUart                = true;
	config->flags.enableLeds                = true;
	config->flags.ledInverted               = false;
	config->flags.dimmerTempInverted        = false;
	config->flags.usesNfcPins               = true;

	config->deviceType                      = DEVICE_CROWNSTONE_OUTLET;

	// All values below are set to something rather than nothing, but are not truly in use.
	config->voltageOffset[GAIN_SINGLE]      = 1000;
	config->currentOffset[GAIN_LOW]         = 1000;
	config->currentOffset[GAIN_HIGH]        = 1000;
	config->powerOffsetMilliWatt            = 0;

	// ADC values [0, 4095] map to [0, 3.6V].
	config->voltageAdcRangeMilliVolt        = 3600;
	config->currentAdcRangeMilliVolt        = 3600;

	config->pwmTempVoltageThreshold         = 2.0;
	config->pwmTempVoltageThresholdDown     = 1.0;

	config->minTxPower                      = -40;

	config->scanWindowUs                    = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold = -35;
}
