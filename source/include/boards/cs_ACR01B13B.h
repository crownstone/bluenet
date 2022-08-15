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
 * Crownstone Built-in Two
 * *******************************************************************************************************************/

/**
 * The built-in two has an nRF52833 chipset. The pin layout however depends on the actual form factor being used,
 * for example the aQFN73, the QFN48 or the WLCSP assignments are different.
 */
void asACR01B13B(boards_config_t* config) {
	config->pinDimmer                             = 15;
	config->pinEnableDimmer                       = PIN_NONE;
	config->pinRelayOn                            = 17;
	config->pinRelayOff                           = 18;

	config->pinAinCurrent[GAIN_LOW]               = GpioToAin(3);
	config->pinAinCurrent[GAIN_HIGH]              = GpioToAin(2);

	config->pinAinVoltage[GAIN_LOW]               = GpioToAin(31);
	config->pinAinVoltage[GAIN_HIGH]              = GpioToAin(28);

	config->pinAinVoltageAfterLoad[GAIN_LOW]      = GpioToAin(30);
	config->pinAinVoltageAfterLoad[GAIN_HIGH]     = GpioToAin(29);

	config->pinAinZeroRef                         = GpioToAin(4);
	config->pinAinDimmerTemp                      = GpioToAin(5);

	config->pinCurrentZeroCrossing                = 11;
	config->pinVoltageZeroCrossing                = GetGpioPin(1, 9);

	config->pinRx                                 = 10;
	config->pinTx                                 = 9;

	config->pinGpio[3]                            = 20;

	config->flags.dimmerInverted                  = true;
	config->flags.enableUart                      = false;
	config->flags.enableLeds                      = false;
	config->flags.dimmerTempInverted              = true;
	config->flags.usesNfcPins                     = true;
	config->flags.hasAccuratePowerMeasurement     = true;
	config->flags.canTryDimmingOnBoot             = true;
	config->flags.canDimOnWarmBoot                = true;
	config->flags.dimmerOnWhenPinsFloat           = true;

	config->deviceType                            = DEVICE_CROWNSTONE_BUILTIN_TWO;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageAdcRangeMilliVolt              = 1800;
	config->currentAdcRangeMilliVolt              = 1800;

	// TODO: All the following values have to be calculated still, now set to some guessed values.
	config->voltageMultiplier[GAIN_LOW]           = -0.25;
	config->voltageMultiplier[GAIN_HIGH]          = -0.25;

	config->voltageAfterLoadMultiplier[GAIN_LOW]  = -0.25;
	config->voltageAfterLoadMultiplier[GAIN_HIGH] = -0.25;

	config->currentMultiplier[GAIN_LOW]           = 0.01;
	config->currentMultiplier[GAIN_HIGH]          = 0.01;

	config->voltageOffset[GAIN_LOW]               = 0;
	config->voltageOffset[GAIN_HIGH]              = 0;

	config->currentOffset[GAIN_LOW]               = 0;
	config->currentOffset[GAIN_HIGH]              = 0;

	config->powerOffsetMilliWatt                  = 0;

	config->pwmTempVoltageThreshold               = 0.2;
	config->pwmTempVoltageThresholdDown           = 0.18;

	config->minTxPower                            = -20;

	config->scanWindowUs                          = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold       = -35;
}
