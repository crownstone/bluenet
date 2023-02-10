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
 * This is the dongle with a nRF52840 on it.
 *
 * Note that the JTAG connector doesn't have UART pins broken out. From what I know, there are also no GPIOs available
 * over USB at first sight. Of course, that's not true because the USB bootloader uses USB. However, it is not easy
 * to just configure RX and TX pins here and use USB.
 *
 * Check components/boards/pca10059.h for pin info.
 *
 * More resources:
 * https://devzone.nordicsemi.com/guides/short-range-guides/b/getting-started/posts/nrf52840-dongle-programming-tutorial
 * https://infocenter.nordicsemi.com/pdf/nRF52_DK_User_Guide_v1.2.pdf
 *
 * Note that the nRF52840 chip on the dongle hardware is configured in high voltage mode.
 *
 * The firmware must set REGOUT0 to 3 V (this is not yet implemented).
 */
void asPca10059(boards_config_t* config) {
	config->pinDimmer                       = PIN_NONE;
	config->pinRx                           = 8;
	config->pinTx                           = 6;

	config->flags.enableUart                = true;
	config->flags.enableLeds                = true;
	// Check this
	//config->flags.usesNfcPins               = true;
	
	config->pinButton[0]                    = GetGpioPin(1,6);
	
	config->pinLed[0]                       = 6;
	config->pinLed[1]                       = 8;
	config->pinLed[2]                       = GetGpioPin(1, 9);
	config->pinLed[3]                       = 12;

	config->deviceType                      = DEVICE_CROWNSTONE_USB;
	config->minTxPower                      = -40;
	config->scanWindowUs                    = config->scanIntervalUs;
}
