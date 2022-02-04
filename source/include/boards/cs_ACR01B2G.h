/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>

/**
 * The second type of plug that is delivered to Crownstone users.
 *
 * Voltage measurements:
 *   - This plug has an offset as additional input for the ADC. It can do differential measurements.
 *   - The ADC is configured with a range of -1.2V to 1.2V.
 *   - Note, voltageRange=1200 is a bit of misnomer here, the complete range is 2x1200 mV = 2400 mV.
 *   - It means that a gain of 1/2 is used.
 *   - The voltage divider (R12/R13 also in this circuit) centers at 3.3V/2 by two identical 68k resistors.
 *   - The equivalent parallel resistor is 34k.
 *   - The opamp gain becomes 34k/10.3M = 0.00330 (or 302.94x when inverted).
 *   - If we divide 2.4V over 4096-1 we have 0.58608 mV per interval.
 *   - The voltage multiplier of 0.00058608 x 302.94 = 0.17754.
 *   - If the reference voltage is exactly 3.3V/2, we should have zero offset for our voltage.
 *   - If VDD is not exactly 3.3V, this is exactly the same for the voltage divider at the opamp as well.
 *   - Resistor values with accuracies of 1% are used. This can lead to a difference of 33 mV (calculation not shown).
 *   - This would amount to a maximum offset error of 33 / 0.58608 = 56 in ADC values.
 *
 * Temperature:
 *   - Threshold of 0.70 corresponds to about 50°C.
 *   - Threshold of 0.25 corresponds to about 90°C.
 *
 * TODO: Here voltageMultiplier=0.171f is used instead of 0.17754f. Check.
 * TODO: Here voltageZero=-99 is used instead of 0. The offset should be at max 56. Check.
 * TODO: The values used now might be empirical. Replace by theoretic values or tell why the calcluation is incorrect.
 */
void asACR01B2G(boards_config_t* config) {
	config->pinDimmer                          = 8;
	config->pinRelayOn                         = 6;
	config->pinRelayOff                        = 7;

	config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);

	config->pinAinZeroRef                      = GpioToAin(2);
	config->pinAinDimmerTemp                   = GpioToAin(5);

	config->pinRx                              = 20;
	config->pinTx                              = 19;

	config->pinLed[LED_RED]                    = 9;
	config->pinLed[LED_GREEN]                  = 10;

	config->flags.dimmerInverted               = false;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.usesNfcPins                  = false; // Set to true if you want to use the LEDs.
	config->flags.canTryDimmingOnBoot          = false;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	// The following two values are empirically determined, through calibration over 10 production crownstones
	config->voltageMultiplier[GAIN_SINGLE]     = 0.171f;
	config->currentMultiplier[GAIN_SINGLE]     = 0.00385f;

	// Calibrated by noisy data from 1 crownstone
	// The following two values are empirically determined, through calibration over noisy data from 1 Crownstone.
	config->voltageOffset[GAIN_SINGLE]         = -99;
	config->currentOffset[GAIN_SINGLE]         = -270;

	// The following value is empirically determined, through calibration over 10 production crownstones
	config->powerOffsetMilliWatt               = 9000;

	// ADC values [-2048, 2047] map to [REF - 1.2V, REF + 1.2V].
	config->voltageAdcRangeMilliVolt           = 1200;

	// ADC values [-2048, 2047] map to [REF - 0.6V, REF + 0.6V].
	config->currentAdcRangeMilliVolt           = 600;

	config->pwmTempVoltageThreshold            = 0.70;
	config->pwmTempVoltageThresholdDown        = 0.25;

	config->minTxPower                         = -20;


	// This board cannot provide enough power for 100% scanning.
	config->scanWindowUs                       = 3 * config->scanIntervalUs / 4;
	config->tapToToggleDefaultRssiThreshold    = -35;
}
