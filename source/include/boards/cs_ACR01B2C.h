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
 * Crownstone Plug Zero
 * *******************************************************************************************************************/

/**
 * The very first plug that is delivered to Crownstone users.
 *
 * Voltage measurements.
 *   - The ACR01B2C has a voltage divider R12/R13 of 82k/18k. This means the opamp will receive input around a voltage
 *     level of 3.3V * 18/(18+82) = 0.594V.
 *   - This is a non-differential measurement. It uses an internal reference voltage of 0.6V.
 *   - The ADC is configured for a range of 0 ... 1.2V. A gain of 1/2 with respect to the 0.6V.
 *     - The other software option would be a reference of VDD/4 (more robust against deviations in true VDD voltage).
 *   - The ADC is configured to use 12-bits, 4096 levels.
 *   - Zero is at 0.594 so 6 mV below 0.6V. If we divide 1.2V over 4096-1 we have 0.2930 mV per interval.
 *   - Hence 6 mV corresponds with 20 levels below 2047 (4096/2-1), which is 2027 (voltageZero should be set to this).
 *   - The resistors R12/R13 when considered parallel have an equivalent resistance of 1/(1/82k+1/18k)=14.76k
 *   - The capacitor at the voltage measurement input is 10 nF and its reactance is 1/(2pi*f*C).
 *   - With 50 Hz this is 1/(2pi*50*10*10-9) = 318 kOhm and with 60 Hz this is 265 kOhm.
 *   - Total voltage divider to calculate gain of opamp is then 10.3M versus 14.76k.
 *   - The opamp gain becomes 14.76k/10.3M = 0.001433 (or 697.83x when inverted).
 *   - The multiplication factor to go from ADC values to V_in is 0.0002930 * 697.83 = 0.20446f.
 *
 * TODO: Here voltageZero=2003 is used instead of 2027. Check.
 * TODO: Here voltageMultiplier=0.2f is used instead of 0.20446f. Check.
 * TODO: The values used now might be empirical. Replace by theoretic values or tell why the calcluation is incorrect.
 */
void asACR01B2C(boards_config_t* config) {
	config->pinDimmer                          = 8;
	config->pinRelayOn                         = 6;
	config->pinRelayOff                        = 7;
	config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);
	config->pinAinDimmerTemp                   = GpioToAin(5);
	config->pinRx                              = 20;
	config->pinTx                              = 19;

	config->pinLed[LED_RED]                    = 10;
	config->pinLed[LED_GREEN]                  = 9;

	config->flags.dimmerInverted               = false;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = false;
	config->flags.usesNfcPins                  = false; // Set to true if you want to use the LEDs.
	config->flags.canTryDimmingOnBoot          = false;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	config->voltageMultiplier[GAIN_SINGLE]     = 0.2f;
	config->currentMultiplier[GAIN_SINGLE]     = 0.0045f;

	// TODO: 2010 seems to be better than 2003 (check this old remark with theory).
	config->voltageOffset[GAIN_SINGLE]         = 2003;
	// TODO: 1991 seems to be better than 1997 (check this old remark with theory).
	config->currentOffset[GAIN_SINGLE]         = 1997;

	config->powerOffsetMilliWatt               = 1500;

	// ADC values [0, 4095] map to [0V, 1.2V].
	config->voltageAdcRangeMilliVolt           = 1200;
	config->currentAdcRangeMilliVolt           = 1200;

	// About 1.5kOhm --> 90-100C
	config->pwmTempVoltageThreshold            = 0.76;
	// About 0.7kOhm --> 70-95C
	config->pwmTempVoltageThresholdDown        = 0.41;

	config->minTxPower                         = -20;

	config->scanWindowUs                       = 3 * config->scanIntervalUs / 4;
	config->tapToToggleDefaultRssiThreshold    = -35;
}
