/*
 * Author: Dominik Egger
 * Copyright: Crownstone
 * Date: 2 Feb., 2017
 * License: LGPLv3+, Apache, and/or MIT, your choice
 *
 * Note: this file is written in C so that it can be used in the bootloader as well.
 */

/**
 * Convention:
 *   * use TABS for identation, but use SPACES for column-wise representations!
 *
 */
#pragma once

#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf52.h"

#ifdef __cplusplus
}
#endif

/**
 *  We use part of the UICR to store information about the hardware board. So the firmware is independent on the
 *  hardware board (we don't need to know the board at compile time). Instead the firmware reads the hardware board
 *  type at runtime from the UICR and assigns the
 */
#define UICR_DFU_INDEX       0 // dfu is using the first 4 bytes in the UICR (0x10001080) for DFU INIT checks
#define UICR_BOARD_INDEX     1 // we use the second 4 bytes in the UICR (0x10001084) to store the hardware board type


//! Current accepted HARDWARE_BOARD types

/*
 * NOTE 1: If you add a new BOARD, also update the function get_hardware_version in
 *         cs_HardwareVersions.h
 */
/*
 * NOTE 2: If you add a new BOARD that has a different pin layout or other board changes
 *         define a new function and add it as a switch case to the configure_board function
 */

#define CROWNSTONE5          0
#define PLUGIN_FLEXPRINT_01  1

// NORDIC DEV BOARDS
#define PCA10036             40
#define PCA10040             41

// the square guidestones from china
#define GUIDESTONE           100

// CROWNSTONE BUILTINS
#define ACR01B1A             1000
#define ACR01B1B             1001
#define ACR01B1C             1002
#define ACR01B1D             1003
#define ACR01B1E             1004

// CROWNSTONE PLUGS
#define ACR01B2A             1500
#define ACR01B2B             1501
#define ACR01B2C             1502

//! BE SURE TO UPDATE cs_HardwareVersions.h IF YOU ADD A NEW BOARD!!

typedef struct  {
	uint32_t hardwareBoard;

	uint8_t pinGpioPwm;
	uint8_t pinGpioRelayOn;
	uint8_t pinGpioRelayOff;
	uint8_t pinAinCurrent;
	uint8_t pinAinVoltage;
	uint8_t pinGpioRx;
	uint8_t pinGpioTx;
	uint8_t pinLedRed;
	uint8_t pinLedGreen;

	struct {
		bool hasRelay: 1;
		bool pwmInverted: 1;
		bool hasSerial: 1;
		bool hasLed : 1;
	} flags;

	uint8_t deviceType;

	float voltageMultiplier;
	float currentMultiplier;
	int32_t voltageZero;
	int32_t currentZero;
	int32_t powerZero;

} boards_config_t;

uint32_t configure_board(boards_config_t* p_config);

