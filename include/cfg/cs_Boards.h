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


#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "nrf52.h"

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
#define ACR01B2E             1503
#define ACR01B2F             1504

//! BE SURE TO UPDATE cs_HardwareVersions.h IF YOU ADD A NEW BOARD!!

typedef struct  {
	//! the hardware board type (number)
	uint32_t hardwareBoard;

	//! gpio pin to control the igbt
	uint8_t pinGpioPwm;
	//! gpio pin to switch the relay on
	uint8_t pinGpioRelayOn;
	//! gpio pin to switch the relay off
	uint8_t pinGpioRelayOff;
	//! analog input pin to read the current
	uint8_t pinAinCurrent;
	//! analog input pin to read the voltage
	uint8_t pinAinVoltage;
	//! analog input pin to read 'zero' line for current measurement
	uint8_t pinAinZeroRef;
	//! analog input pin to read the pwm temperature
	uint8_t pinAinPwmTemp;
	//! gpio pin to receive uart
	uint8_t pinGpioRx;
	//! gpio pin to send uart
	uint8_t pinGpioTx;
	//! gpio pin to control the "red" led
	uint8_t pinLedRed;
	//! gpio pin to control the "green" led
	uint8_t pinLedGreen;

	struct __attribute__((__packed__)) {
		//! true if board has relays
		bool hasRelay: 1;
		//! true if the pwm is inverted (setting gpio high turns light off)
		bool pwmInverted: 1;
		//! true if the board has serial / uart
		bool hasSerial: 1;
		//! true if the board has leds
		bool hasLed : 1;
		//! true if led off when gpio set high
		bool ledInverted: 1;
	} flags;

	/* device type, e.g. crownstone plug, crownstone builtin, guidestone
	   can be overwritten for debug purposes at compile time, but is otherwise
	   determined by the board type */
	uint8_t deviceType;

	//! tbd
	float voltageMultiplier;
	//! tbd
	float currentMultiplier;
	//! tbd
	int32_t voltageZero;
	//! tbd
	int32_t currentZero;
	//! tbd
	int32_t powerZero;

	/* the minimum radio transmission power to be used, e.g. builtins need higher
	   tx power because of surrounding metal */
	int8_t minTxPower;

	//! Voltage of PWM thermometer at which the PWM is too hot
	float pwmTempVoltageThreshold;
	//! Voltage of PWM thermometer at which the PWM is cool enough again
	float pwmTempVoltageThresholdDown;

} boards_config_t;

uint32_t configure_board(boards_config_t* p_config);

#ifdef __cplusplus
}
#endif
