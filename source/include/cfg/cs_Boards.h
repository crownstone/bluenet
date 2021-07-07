/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 2 Feb., 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 *
 * NOTE 1: This file is written in C so that it can be used in the bootloader as well.
 *
 * NOTE 2: If you add a new BOARD, also update the function get_hardware_version in cs_HardwareVersions.h.
 *
 * NOTE 3: If you add a new BOARD that has a different pin layout or other board changes, define a new function and
 * add it as a switch case to the configure_board function
 *
 * NOTE 4: This is the only place where the versions are maintained. There is a separate cmake module that knows
 * how to extract the BOARD_VERSION from this header file. It uses a particular regular expression. If you add
 * weird define statements, this module might choke on that.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 *  We use part of the UICR to store information about the hardware board. So the firmware is independent on the
 *  hardware board (we don't need to know the board at compile time). Instead the firmware reads the hardware board
 *  type at runtime from the UICR and assigns the
 */


// Nordic dev board, but with some hardware bugs.
#define PCA10036             40
// Nordic dev board in active use.
#define PCA10040             41
// Nordic dev board for the nRF52833
#define PCA10100             42

// Rectangular beacons from China.
#define GUIDESTONE           100


// Crownstone USB dongle
#define CS_USB_DONGLE        200

// CROWNSTONE BUILTINS

// Prototype builtin. Not in production.
#define ACR01B1A             1000
// Prototype builtin. Not in production.
#define ACR01B1B             1001
// Prototype builtin. Not in production.
#define ACR01B1C             1002
#define ACR01B1D             1003 // released
#define ACR01B1E             1004 // same as ACR01B1D, but with patch
#define ACR01B10B            1007 // Builtin One v20
#define ACR01B10D            1008 // Builtin One v25

#define ACR01B13B            1009 // Builtin Two development version

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
#define ACR01B2G             1504
	
// Crownstone Plug One (first prototype of second edition of the plug)
#define ACR01B11A            1505

/** Board configuration
 *
 * Configure pins for control relays, IGBTs, LEDs, UART, current sensing, etc.
 */
typedef struct  {
	//! The hardware board type (number).
	uint32_t hardwareBoard;

	//! GPIO pin to control the IGBTs.
	uint8_t pinGpioPwm;

	//! GPIO pin to enable the IGBT circuit.
	uint8_t pinGpioEnablePwm;

	//! GPIO pin to switch the relay on.
	uint8_t pinGpioRelayOn;

	//! GPIO pin to switch the relay off.
	uint8_t pinGpioRelayOff;

	//! Analog input pin to read the current with high gain.
	uint8_t pinAinCurrentGainHigh;

	//! Analog input pin to read the current with medium gain.
	uint8_t pinAinCurrentGainMed;

	//! Analog input pin to read the current with low gain.
	uint8_t pinAinCurrentGainLow;

	//! Analog input pin to read the voltage.
	uint8_t pinAinVoltage;

//	//! Analog input pin to read the voltage.
//	uint8_t pinAinVoltageGainHigh;

//	//! Analog input pin to read the voltage.
//	uint8_t pinAinVoltageGainLow;

	//! Analog input pin to read 'zero' line for current and voltage measurement (optional).
	uint8_t pinAinZeroRef;

	//! Analog input pin to read the pwm temperature.
	uint8_t pinAinPwmTemp;

	//! GPIO pin to receive uart.
	uint8_t pinGpioRx;

	//! GPIO pin to send uart.
	uint8_t pinGpioTx;
	
	//! GPIO custom pins (only four)
	uint8_t pinGpio[4];

	//! Buttons (four on dev. kit)
	uint8_t pinButton[4];

	//! GPIO pin to control the "red" led.
	uint8_t pinLedRed;

	//! GPIO pin to control the "green" led.
	uint8_t pinLedGreen;

	//! Leds as array
	uint8_t pinLed[4];

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

		//! True if the board has a zero ref pin for current and voltage measurements.
		bool hasAdcZeroRef: 1;

		//! True if the temperature sensor of the dimmer is inverted (NTC).
		bool pwmTempInverted: 1;
	} flags;

	/** Device type, e.g. crownstone plug, crownstone builtin, guidestone.
	 *
	 * This can be overwritten for debug purposes at compile time, but is otherwise determined by the board type.
	 */
	uint8_t deviceType;

	//! Multiplication factor for current measurement.
	float currentMultiplier;

	//! Multiplication factor for voltage measurement.
	float voltageMultiplier;

	//! Offset for voltage measurement.
	int32_t voltageZero;

	//! Offset for current measurement.
	int32_t currentZero;

	//! Measured power when there is no load (mW).
	int32_t powerZero;

	//! Default range in mV on the voltage pin. Example: if default should be [-0.6V, 0.6V] then voltageRange is 1200.
	uint32_t voltageRange;

	//! Default range in mV on the current pin. Example: if default should be [0V, 3.6V] then currentRange is 3600.
	uint32_t currentRange;

	/** The minimum radio transmission power to be used.
	 *
	 * Builtins need higher TX power than plugs because of surrounding metal and wall material.
	 */
	int8_t minTxPower;

	//! Voltage of PWM thermometer at which the dimmer is too hot.
	float pwmTempVoltageThreshold;

	//! Voltage of PWM thermometer at which the dimmer is cool enough again.
	float pwmTempVoltageThresholdDown;

	/**
	 * Scan interval in μs.
	 *
	 * Since this setting is also used when connecting, it influences the time it takes to make an outgoing connection.
	 * For some reason, if the scan interval is 2s, it takes at least 2s before a connection is made.
	 *
	 * It's also best to make this interval not a multiple of the advertising interval, because in that case, it can happen
	 * that the advertisements are outside the scan window every time.
	 *
	 * Must not be larger than (1000 * BEARER_SCAN_INT_MAX_MS).
	 */
	uint32_t scanIntervalUs;

	/**
	 * Scan window in μs.
	 *
	 * Must not be larger than scan interval, and not smaller than (1000 * BEARER_SCAN_WIN_MIN_MS).
	 * See https://devzone.nordicsemi.com/f/nordic-q-a/14733/s132-scan-interval-window-adv-interval
	 */
	uint32_t scanWindowUs;

	/**
	 * Default RSSI threshold, above which tap to toggle will trigger.
	 */
	int8_t tapToToggleDefaultRssiThreshold;

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
