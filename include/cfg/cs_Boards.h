/*
 * Author: Crownstone Team
 * Copyright: Crownstone
 * Date: 2 Feb., 2017
 * License: LGPLv3+, Apache, and/or MIT, your choice
 *
 * NOTE 1: This file is written in C so that it can be used in the bootloader as well.
 *
 * NOTE 2: If you add a new BOARD, also update the function get_hardware_version in cs_HardwareVersions.h.
 *
 * NOTE 3: If you add a new BOARD that has a different pin layout or other board changes, define a new function and 
 * add it as a switch case to the configure_board function
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

// DFU is using the first 4 bytes in the UICR (0x10001080) for DFU INIT checks.
#define UICR_DFU_INDEX       0 
// We use the second 4 bytes in the UICR (0x10001084) to store the hardware board type.
#define UICR_BOARD_INDEX     1 

// Crownstone 5 is one of the first Crownstone types. Not in production.
#define CROWNSTONE5          0
// A plug with a flexible print. Not in production.
#define PLUGIN_FLEXPRINT_01  1

// Nordic dev board, but with some hardware bugs.
#define PCA10036             40
// Nordic dev board in active use.
#define PCA10040             41

// Rectangular beacons from China.
#define GUIDESTONE           100

// CROWNSTONE BUILTINS

// Prototype builtin. Not in production.
#define ACR01B1A             1000
// Prototype builtin. Not in production.
#define ACR01B1B             1001
// Prototype builtin. Not in production.
#define ACR01B1C             1002
#define ACR01B1D             1003
#define ACR01B1E             1004
#define ACR01B6A             1005

// CROWNSTONE PLUGS

// Prototype plug.
#define ACR01B2A             1500
// Prototype plug. Replace caps before and after LDO with electrolytic caps due to DC bias.
#define ACR01B2B             1501

// Production release plug. Replace primary supply registor with MELF type and increase value to 100 Ohm.
#define ACR01B2C             1502

// Prototype plug. Remove MELF. Add compensation cap. Remove C7. Add pull-down resistor to LGBT driver input.
// Move thermal fuse to cover both power paths. Alter offset and gain of power measurement service. Add measurement
// offset to ADC.
#define ACR01B2E             1503
// Schematic change. Change power measurement resistor values.
#define ACR01B2F             1504

/** Board configuration
 *
 * Configure pins for control relays, IGBTs, LEDs, UART, current sensing, etc.
 */
typedef struct  {
	//! The hardware board type (number).
	uint32_t hardwareBoard;

	//! GPIO pin to control the IGBTs.
	uint8_t pinGpioPwm;
	//! GPIO pin to switch the relay on.
	uint8_t pinGpioRelayOn;
	//! GPIO pin to switch the relay off.
	uint8_t pinGpioRelayOff;
	//! Analog input pin to read the current.
	uint8_t pinAinCurrent;
	//! Analog input pin to read the voltage.
	uint8_t pinAinVoltage;
	//! Analog input pin to read 'zero' line for current measurement.
	uint8_t pinAinZeroRef;
	//! Analog input pin to read the pwm temperature.
	uint8_t pinAinPwmTemp;
	//! GPIO pin to receive uart.
	uint8_t pinGpioRx;
	//! GPIO pin to send uart.
	uint8_t pinGpioTx;
	//! GPIO pin to control the "red" led.
	uint8_t pinLedRed;
	//! GPIO pin to control the "green" led.
	uint8_t pinLedGreen;

	//! Flags about pin order, presence of components, etc.
	struct __attribute__((__packed__)) {
		//! True if board has relays.
		bool hasRelay: 1;
		//! True if the pwm is inverted (setting gpio high turns light off).
		bool pwmInverted: 1;
		//! True if the board has serial / uart.
		bool hasSerial: 1;
		//! True if the board has leds.
		bool hasLed : 1;
		//! True if led off when GPIO set high.
		bool ledInverted: 1;
	} flags;

	/** Device type, e.g. crownstone plug, crownstone builtin, guidestone.
	 *
	 * This can be overwritten for debug purposes at compile time, but is otherwise determined by the board type.
	 */
	uint8_t deviceType;

	//! Multiplication factor for voltage measurement.
	float voltageMultiplier;
	//! Multiplication factor for current measurement.
	float currentMultiplier;
	//! Offset for voltage measurement.
	int32_t voltageZero;
	//! Offset for current measurement.
	int32_t currentZero;
	//! Offset for power measurement.
	int32_t powerZero;

	/** The minimum radio transmission power to be used.
	 * 
	 * Builtins need higher TX power than plugs because of surrounding metal and wall material.
	 */
	int8_t minTxPower;

	//! Voltage of PWM thermometer at which the PWM is too hot.
	float pwmTempVoltageThreshold;
	//! Voltage of PWM thermometer at which the PWM is cool enough again.
	float pwmTempVoltageThresholdDown;

} boards_config_t;

/** Configure board.
 *
 * This function reads a board type id from UICR. This is a dedicated part in memory that is preserved across firmware
 * updates and set only once in the factory. Using this board type id, the p_config parameter is filled with the
 * relevant values. If the UICR has not been written before it will read 0xFFFFFFFF and a default board is chosen,
 * see implementation.
 *
 * @param p_config                               configuration to be populated
 * @return                                       error value (NRF_SUCCESS or NRF_ERROR_INVALID_PARAM)
 */
uint32_t configure_board(boards_config_t* p_config);

#ifdef __cplusplus
}
#endif
