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
 * This prototype is based on the nRF52840 (in contrast with the ACR01B13B above which is based on the nRF52833).
 */
void asACR01B15A(boards_config_t* config) {
	config->pinDimmer                          = GetGpioPin(1, 13);  // Gate-N
	config->pinEnableDimmer                    = GetGpioPin(1, 14);  // Gate-P

	config->pinRelayOn                         = 13;
	config->pinRelayOff                        = 15;

	config->pinAinCurrent[GAIN_LOW]            = GpioToAin(2);
	config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(30);

	config->pinAinVoltage[GAIN_LOW]            = GpioToAin(28);
	config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(3);

	config->pinAinVoltageAfterLoad[GAIN_LOW]   = GpioToAin(29);
	config->pinAinVoltageAfterLoad[GAIN_HIGH]  = GpioToAin(31);

	config->pinAinZeroRef                      = GpioToAin(5);
	config->pinAinDimmerTemp                   = GpioToAin(4);

	config->pinCurrentZeroCrossing             = 8;
	config->pinVoltageZeroCrossing             = GetGpioPin(1, 9);

	config->pinRx                              = 22;
	config->pinTx                              = 20;

	config->pinGpio[0]                         = 24;
	config->pinGpio[1]                         = GetGpioPin(1, 0);
	config->pinGpio[2]                         = GetGpioPin(1, 2);
	config->pinGpio[3]                         = GetGpioPin(1, 4);
	config->pinGpio[4]                         = GetGpioPin(1, 6);
	config->pinGpio[5]                         = GetGpioPin(1, 7);
	config->pinGpio[6]                         = 9;
	config->pinGpio[7]                         = 16;
	config->pinGpio[8]                         = 21;
	config->pinGpio[9]                         = GetGpioPin(1, 15);

	config->flags.dimmerInverted               = true;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.usesNfcPins                  = true;
	config->flags.canTryDimmingOnBoot          = true;
	config->flags.canDimOnWarmBoot             = true;
	config->flags.dimmerOnWhenPinsFloat        = false;

	config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN_TWO;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageRange                       = 1800;
	config->currentRange                       = 1800;

	//TODO: All the following values have to be calculated still, now set to some guessed values.
	config->voltageMultiplier[GAIN_LOW]        = -0.25;
	config->voltageMultiplier[GAIN_HIGH]       = -0.25;

	config->voltageAfterLoadMultiplier[GAIN_LOW]  = -0.25;
	config->voltageAfterLoadMultiplier[GAIN_HIGH] = -0.25;

	config->currentMultiplier[GAIN_LOW]        = 0.01;
	config->currentMultiplier[GAIN_HIGH]       = 0.01;

	config->voltageZero[GAIN_LOW]              = 0;
	config->voltageZero[GAIN_HIGH]             = 0;

	config->currentZero[GAIN_LOW]              = 0;
	config->currentZero[GAIN_HIGH]             = 0;

	config->powerZero                          = 0;

	config->pwmTempVoltageThreshold            = 0.2;
	config->pwmTempVoltageThresholdDown        = 0.18;

	config->minTxPower                         = -20;

	config->scanWindowUs                       = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold    = -35;
}
