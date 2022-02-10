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
 * Crownstone Plug One
 * *******************************************************************************************************************/

/*
 * This plug has not been delivered to customers.
 *
 * This design has an improved power design. It has two opamps for both voltage and current.
 *
 * Remarks:
 *   - Make sure that pin P0.19 is floating (not listed here). It is a short to another pin (P0.30).
 *   - The pin for the dimmer is accidentally connected to a non-connected pin (A18).
 *   - It has been patched to pin LED red on pin 8.
 *
 * Temperature is set according to:
 *   T_0    = 25 (room temperature)
 *   R_0    = 10000 (from datasheet, 10k at 25 degrees Celcius)
 *   B_ntc  = 3434 K (from datasheet of NCP15XH103J03RC at 25-85 degrees Celcius)
 *   B_ntc  = 3380 K (if we assume it to stay cold at for 25-50 degrees Celcius)
 *   B_ntc  = 3455 K (if we assume it to become hot at 25-100 degrees Celcius)
 *   R_22   = 15000
 *   R_ntc  = R_0 * exp(B_ntc * (1/(T+273.15) - 1/(T_0+273.15)))
 *   V_temp = 3.3 * R_ntc/(R_22+R_ntc)
 *
 * We want to trigger between 76 < T < 82 degrees Celcius.
 *
 *   |  T | R_ntc | V_temp | B_ntc |
 *   | -- | ----  | ------ | ----- |
 *   | 25 | 10000 | 1.32   |       |
 *   | 76 |  1859 | 0.3639 |       | <- used value
 *   | 82 |  1575 | 0.3135 |       | <- used value
 *   | 90 |  1256 | 0.2551 |  3455 |
 *
 * Hence, 0.3135 < V_temp < 0.3639 in an inverted fashion (higher voltage means cooler).
 *
 * The voltage range with R10 = 36.5k and R6 = 10M (C9 neglectable) is 0.0365/(0.0365+10)*680 = 2.47V.
 * Thus it is 1.24V each way. A range of 1300 would be fine. Then 1240 would correspond to 240V RMS.
 * Hence, the voltage multiplier (from ADC values to volts RMS) is 0.19355.
 */
void asACR01B11A(boards_config_t* config) {
	config->pinDimmer                          = 8; // Actually red LED, but the dimmer pin is N.C.
	config->pinRelayOn                         = 15;
	config->pinRelayOff                        = 13;

	config->pinAinCurrent[GAIN_LOW]            = GpioToAin(2);
	config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(3);

	config->pinAinVoltage[GAIN_LOW]            = GpioToAin(31);
	config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(29);

	config->pinAinZeroRef                      = GpioToAin(5);
	config->pinAinDimmerTemp                   = GpioToAin(4);

	config->pinRx                              = 22;
	config->pinTx                              = 20;

	config->pinGpio[0]                         = 24;
	config->pinGpio[1]                         = GetGpioPin(1, 0);
	config->pinGpio[2]                         = GetGpioPin(1, 2);
	config->pinGpio[3]                         = GetGpioPin(1, 4);

	config->pinLed[LED_RED]                    = 8;
	config->pinLed[LED_GREEN]                  = GetGpioPin(1, 9);

	config->flags.dimmerInverted               = true;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.usesNfcPins                  = false;
	config->flags.canTryDimmingOnBoot          = false;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_PLUG_ONE;

	// All the values below are just copied from configuration values from other hardware and should be adjusted.
	config->voltageMultiplier[GAIN_SINGLE]     = 0.19355f;
	config->currentMultiplier[GAIN_SINGLE]     = 0.00385f;
	config->voltageOffset[GAIN_SINGLE]         = 0;

	config->currentOffset[GAIN_LOW]            = -270;
	config->currentOffset[GAIN_HIGH]           = -270;

	config->powerOffsetMilliWatt               = 9000;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageAdcRangeMilliVolt           = 1800;

	// ADC values [-2048, 2047] map to [REF - 0.6V, REF + 0.6V].
	config->currentAdcRangeMilliVolt           = 600;

	config->pwmTempVoltageThreshold            = 0.3639;
	config->pwmTempVoltageThresholdDown        = 0.3135;

	config->minTxPower                         = -20;

	config->scanWindowUs                       = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold    = -35;
}
