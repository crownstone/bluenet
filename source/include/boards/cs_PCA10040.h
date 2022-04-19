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
 * This is the development board for the nRF52832. Certain pins have multiple functions, for example:
 *
 *   + P0.05 = UART_RTS
 *   + P0.06 = UART_TXD
 *   + P0.07 = UART_CTS
 *   + P0.08 = UART_RXD
 *   + P0.13 = BUTTON1
 *   + P0.14 = BUTTON2
 *   + P0.15 = BUTTON3
 *   + P0.16 = BUTTON4
 *   + P0.17 = LED1
 *   + P0.18 = LED2
 *   + P0.19 = LED3
 *   + P0.20 = LED4
 *
 * There is also default Arduino routing. For example:
 *
 *   + P0.02 = AREF
 *   + P0.24 = D12
 *   + P0.25 = D13
 *   + P0.26 = SDA
 *   + P0.27 = SCL
 *
 * https://infocenter.nordicsemi.com/pdf/nRF52_DK_User_Guide_v1.2.pdf
 *
 * The voltageMultiplier and currentMultiplier values are set to 0.0 which disables checks with respect to sampling.
 */
void asPca10040(boards_config_t* config) {
	config->pinDimmer                          = 20;
//	config->pinEnableDimmer                    = 22;
	config->pinRelayOn                         = 11;
	config->pinRelayOff                        = 12;
	config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(3);
	config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(4);
//	config->pinAinZeroRef                      = GpioToAin(28);
//	config->pinAinVoltage[GAIN_MIDDLE]         = GpioToAin(29);
//	config->pinAinCurrent[GAIN_MIDDLE]         = GpioToAin(30);
//	config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(31);
	config->pinAinDimmerTemp                   = GpioToAin(2);

	config->pinRx                              = 8;
	config->pinTx                              = 6;

	config->pinGpio[0]                         = 27;  // Also SCL
	config->pinGpio[1]                         = 26;  // Also SDA
	config->pinGpio[2]                         = 25;
	config->pinGpio[3]                         = 24;

	config->pinButton[0]                       = 13;
	config->pinButton[1]                       = 14;
	config->pinButton[2]                       = 15;
	config->pinButton[3]                       = 16;

	config->pinLed[0]                          = 17;
	config->pinLed[1]                          = 18;
	config->pinLed[2]                          = 19;
//	config->pinLed[3]                          = 20; // Already used as dimmer

	config->flags.dimmerInverted               = true;
	config->flags.enableUart                   = true;
	config->flags.enableLeds                   = true;
	config->flags.ledInverted                  = true;
	config->flags.dimmerTempInverted           = false;
	config->flags.usesNfcPins                  = false;
//	config->flags.canTryDimmingOnBoot          = false;
//	config->flags.canDimOnWarmBoot             = false;
//	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	// All values below are set to something rather than nothing, but are not truly in use.
	config->voltageOffset[GAIN_SINGLE]         = 1000;
	config->currentOffset[GAIN_SINGLE]         = 1000;
	config->powerOffsetMilliWatt               = 0;

	// ADC values [0, 4095] map to [0, 3.6V].
	config->voltageAdcRangeMilliVolt           = 3600;
	config->currentAdcRangeMilliVolt           = 3600;

	config->pwmTempVoltageThreshold            = 2.0;
	config->pwmTempVoltageThresholdDown        = 1.0;

	config->minTxPower                         = -40;

	config->scanWindowUs                       = 3 * config->scanIntervalUs / 4;
	config->tapToToggleDefaultRssiThreshold    = -35;
}
