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

//! Current accepted HARDWARE_BOARD types

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

// CROWNSTONE PLUGS
#define ACR01B2A             1500
#define ACR01B2B             1501
#define ACR01B2C             1502

typedef struct  {
	uint32_t hardwareBoard;

	uint8_t pinGpioPwm;
	uint8_t pinGpioRelayOn;
	uint8_t pinGpioRelayOff;
	uint8_t pinAinCurrent;
	uint8_t pinAinVoltage;
	uint8_t pinAinPwmTemp;
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

	float pwmTempVoltageThreshold; //! Voltage of PWM thermometer at which the PWM is too hot
	float pwmTempVoltageThresholdDown; //! Voltage of PWM thermometer at which the PWM is cool enough again

} boards_config_t;

uint32_t configure_board(boards_config_t* p_config);

