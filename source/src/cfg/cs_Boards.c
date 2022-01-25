/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 20, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

/* Hardware board configuration.
 *
 * This file stores information that is different per type of hardware. Not every type of hardware is available to
 * customers. It contains the following information:
 *   - pin assignment
 *   - existence of e.g. dimming/switch hardware (a Guidestone or dongle does not have relays or IGBTs)
 *   - circuit parameters (e.g. the measurement circuit might have different values)
 *   - calibration values (e.g. the threshold for triggering a high temperature warning depends on board layout, or
 *     the value with which we want to trigger a tap-to-toggle action depends on the antenna)
 *
 * For information on how to add a new board see:
 *    - https://github.com/crownstone/bluenet/blob/master/docs/ADD_BOARD.md
 *
 * The hardware that is available to customers:
 *
 *    - ACR01B1D, the Crownstone Built-in Zero
 *    - ACR01B10D, the Crownstone Built-in One
 *    - ACR01B2C, the Crownstone Plug
 *    - ACR01B2G, the Crownstone Plug with some electronic improvements
 *    - Guidestone, one version
 *    - USB dongle, one version
 *
 * The hardware that is in development:
 *
 *    - ACR01B15A, the Crownstone Built-in Two
 */

#include "cfg/cs_Boards.h"
#include "cfg/cs_DeviceTypes.h"
#include "nrf_error.h"
#include "nrf52.h"
#include "cfg/cs_AutoConfig.h"

/**
 * Initialize conservatively (as if given pins are not present).
 */
void init(boards_config_t* config) {
	config->pinDimmer = PIN_NONE;
	config->pinEnableDimmer= PIN_NONE;
	config->pinRelayOn = PIN_NONE;
	config->pinRelayOff = PIN_NONE;
	config->pinAinZeroRef = PIN_NONE;
	config->pinAinDimmerTemp = PIN_NONE;
	config->pinCurrentZeroCrossing = PIN_NONE;
	config->pinVoltageZeroCrossing = PIN_NONE;
	config->pinRx = PIN_NONE;
	config->pinTx = PIN_NONE;
	config->deviceType = DEVICE_UNDEF;
	config->powerZero = 0;
//	config->voltageRange = ;
//	config->currentRange = ;
	config->minTxPower = 0;
//	config->pwmTempVoltageThreshold = ;
//	config->pwmTempVoltageThresholdDown = ;
//	config->scanIntervalUs = ;
//	config->scanWindowUs = ;
//	config->tapToToggleDefaultRssiThreshold = ;

//	config->flags.dimmerInverted = ;
	config->flags.enableUart = false;
	config->flags.enableLeds = false;
//	config->flags.ledInverted = ;
//	config->flags.dimmerTempInverted = ;
	config->flags.usesNfcPins = false;
	config->flags.canTryDimmingOnBoot = false;
	config->flags.canDimOnWarmBoot = false;
	config->flags.dimmerOnWhenPinsFloat = true;

	for (uint8_t i = 0; i < GAIN_COUNT; ++i) {
		config->pinAinVoltage[i] = PIN_NONE;
		config->pinAinCurrent[i] = PIN_NONE;
		config->pinAinVoltageAfterLoad[i] = PIN_NONE;
		config->voltageMultiplier[i] = 0.0;
		config->voltageAfterLoadMultiplier[i] = 0.0;
		config->currentMultiplier[i] = 0.0;
		config->voltageZero[i] = 0;
		config->voltageAfterLoadZero[i] = 0;
		config->currentZero[i] = 0;
	}
	for (uint8_t i = 0; i < GPIO_INDEX_COUNT; ++i) {
		config->pinGpio[i] = PIN_NONE;
	}
	for (uint8_t i = 0; i < LED_COUNT; ++i) {
		config->pinLed[i] = PIN_NONE;
	}
	for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
		config->pinButton[i] = PIN_NONE;
	}
}

/**
 * Helper function to map GPIO pins to AIN indices.
 */
uint8_t GpioToAinOnChipset(uint8_t gpio, uint8_t chipset) {
	switch(chipset) {
		case CHIPSET_NRF52832:
		case CHIPSET_NRF52833:
		case CHIPSET_NRF52840:
			switch(gpio) {
				case  2: return 0;
				case  3: return 1;
				case  4: return 2;
				case  5: return 3;
				case 28: return 4;
				case 29: return 5;
				case 30: return 6;
				case 31: return 7;
				default:
					return PIN_NONE;
			}
		default:
			return PIN_NONE;
	}
}

// For now mapping is always the same, so this simplified function can be used.
uint8_t GpioToAin(uint8_t gpio) {
	return GpioToAinOnChipset(gpio, CHIPSET_NRF52832);
}

/* ********************************************************************************************************************
 * Crownstone Built-in Zero
 * *******************************************************************************************************************/

/**
 * Most important to know about this board is that it cannot provide enough power for 100% scanning.
 *
 * This Crownstone hardware (a built-in) has a voltage range of 1200 mV (it will be configured with a gain of 1/2).
 * It is either possible to use VDD as reference (VDD/4) or an internal reference (0.6V). It is configured with an
 * internal reference.
 */
void asACR01B1D(boards_config_t* config) {
	config->pinDimmer                          = 8;
	config->pinRelayOn                         = 6;
	config->pinRelayOff                        = 7;
	config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);
	config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	config->pinAinDimmerTemp                   = GpioToAin(5);

	config->pinRx                              = 20;
	config->pinTx                              = 19;
	config->pinLed[LED_RED]                    = 10;
	config->pinLed[LED_GREEN]                  = 9;

	config->flags.dimmerInverted               = false;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = true;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = false;
	config->flags.usesNfcPins                  = false; // Set to true if you want to use the LEDs.
	config->flags.canTryDimmingOnBoot          = false;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN;

	// TODO: Explain this value
	config->voltageMultiplier[GAIN_SINGLE]     = 0.2f;
	
	// TODO: Explain this value
	config->currentMultiplier[GAIN_SINGLE]     = 0.0044f;
	
	// TODO: Explain this value
	config->voltageZero[GAIN_SINGLE]           = 1993;
	
	// TODO: Explain this value
	config->currentZero[GAIN_SINGLE]           = 1980;

	// TODO: Explain this value
	config->powerZero                          = 3500;

	// ADC values [0, 4095] map to [0V, 1.2V].
	config->voltageRange                       = 1200;
	config->currentRange                       = 1200;

	// TODO: Explain these comments. About 1.5kOhm --> 90-100C
	config->pwmTempVoltageThreshold            = 0.76;
	// TODO: Explain these comments. About 0.7kOhm --> 70-95C
	config->pwmTempVoltageThresholdDown        = 0.41;

	config->minTxPower                         = -20;

	// This board cannot provide enough power for 100% scanning!
	// So set an interval that's not in sync with advertising interval.
	// And a scan window of 75% of the interval.
	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = 105 * 1000;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

/* ********************************************************************************************************************
 * Crownstone Built-in One
 * *******************************************************************************************************************/

/**
 * The ACR01B10D is also known as the Crownstone Built-in One. It comes with version v20 or the v25, but they are
 * similar from a board configuration perspective.
 *
 * The PWM enable pin might not be connected on the v20, but it is fine to be set nevertheless.
 *
 * Reference voltage
 *   - The reference voltage can be measured on pin P0.02 or (AIN0).
 *   - It is created by a ADR280-1V2, which provides a stable 1.2V.
 *
 * Measure current
 *   - There are three possible gains to measure current. Mainly the lowest gain is used.
 *   - There is a zero reference for the current (and voltage) and measures therefore differential.
 *
 * Measure voltage
 *   - This built-in has an offset as additional input for the ADC. It can do differential measurements.
 *   - The ADC is configured with a range of -1.8V to 1.8V.
 *   - Said otherwise, the ADC is configured with 1/3 gain.
 *   - The voltage divider (R12/R13) centers at 3.3V/2 through two identical 68k resistors.
 *   - The equivalent parallel resistor is 34k.
 *   - The opamp gain becomes 34k/10.3M = 0.00330 (or 302.94x when inverted).
 *   - If we divide 3.6V over 4096-1 we have 0.87912 mV per interval.
 *   - The voltage multiplier of 0.00087912 * 302.94 = 0.26632.
 *   - The difference between 3.3V/2=1.65V and 1.2V = 0.45V.
 *   - That means there will be an offset of 450 / 0.87912 = 512. The voltageZero value.
 * See: https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fsaadc.html&cp=4_0_0_5_22_2&anchor=saadc_digital_output
 *
 * TODO: Explain how it relates to:
 *       RESULT = (V(P) – V(N)) * (GAIN/REFERENCE) * 2^(RESOLUTION - m)
 *
 * Temperature is set according to
 *   T_0    = 25 (room temperature)
 *   R_0    = 10000 (from datasheet, 10k at 25 degrees Celcius)
 *   B_ntc  = 3434 K (from datasheet of NCP15XH103J03RC at 25-85 degrees Celcius)
 *   B_ntc  = 3380 K (if we assume it to stay cold at for 25-50 degrees Celcius)
 *   B_ntc  = 3455 K (if we assume it to become hot at 25-100 degrees Celcius)
 *   R_18   = 18000
 *   R_ntc  = R_0 * exp(B_ntc * (1/(T+273.15) - 1/(T_0+273.15)))
 *   V_temp = 3.3 * R_ntc/(R_22+R_ntc)
 * See https://en.wikipedia.org/wiki/Thermistor#B_or_%CE%B2_parameter_equation.
 *
 * We want to trigger between 76 < T < 82 degrees Celcius.
 *
 *   |  T | R_ntc | V_temp | B_ntc |
 *   | -- | ----  | ------ | ----- |
 *   | 25 | 10000 | 1.32   |       |
 *   | 76 |  1859 | 0.3090 |  3434 | <- TODO: should be the used value
 *   | 82 |  1575 | 0.2655 |  3434 | <- TODO: should be the used value
 *   | 90 |  1256 | 0.2153 |  3455 |
 *
 * Hence, 0.3135 < V_temp < 0.3639 in an inverted fashion (higher voltage means cooler).
 *
 * Extras
 *   - There's a pin which can be used to measure the 15V supply. It has a 324/80.6 voltage divider.
 *   - It should be visible as a steady 15V * 80.6/(80.6+324) = 2.99V input on P0.05.
 *   - The TX power is with -20 higher for the built-ins than for the plugs.
 */
void asACR01B10D(boards_config_t* config) {
	config->pinDimmer                          = 8;
	config->pinEnableDimmer                    = 10;
	config->pinRelayOn                         = 14;
	config->pinRelayOff                        = 13;

	config->pinAinCurrent[GAIN_LOW]            = GpioToAin(30);
	config->pinAinCurrent[GAIN_MIDDLE]         = GpioToAin(29);
	config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(28);

	config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);

	config->pinAinZeroRef                      = GpioToAin(2);
	config->pinAinDimmerTemp                   = GpioToAin(4);

	config->pinRx                              = 20;
	config->pinTx                              = 19;

	config->flags.dimmerInverted               = false;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.usesNfcPins                  = true;
	config->flags.canTryDimmingOnBoot          = true;
	config->flags.canDimOnWarmBoot             = true;
	config->flags.dimmerOnWhenPinsFloat        = false;

	config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN_ONE;

	// TODO: Explain why not -0.26632 (see above). If incorrect, replace.
	config->voltageMultiplier[GAIN_SINGLE]     = -0.2547;

	// TODO: Explain this value
	config->currentMultiplier[GAIN_LOW]        = 0.01486f;

	// TODO: Calculate the following values (now set to something arbitrary)
	config->currentMultiplier[GAIN_MIDDLE]     = 0.015;
	config->currentMultiplier[GAIN_HIGH]       = 0.015;

	// TODO: Explain why not 512 (see above). If 500 incorrect, replace.
	config->voltageZero[GAIN_SINGLE]           = 500;

	// TODO: Explain this value
	config->currentZero[GAIN_LOW]              = -125;

	// TODO: Calculate the following values (now set to something arbitrary)
	config->currentZero[GAIN_MIDDLE]           = -125;
	config->currentZero[GAIN_HIGH]             = -125;

	// TODO: Explain this value
	config->powerZero                          = 800;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageRange                       = 1800;
	// ADC values [-2048, 2047] map to [REF - 1.2V, REF + 1.2V].
	config->currentRange                       = 1200;

	// TODO: These are incorrectly calculated with R_12 = 16k and B_ntc = 3380 K.
	config->pwmTempVoltageThreshold            = 0.35; // 0.3518
	config->pwmTempVoltageThresholdDown        = 0.30; // 0.3036
	
	// TODO: It should be with R_12 = 18k and B_ntc = 3434 K (enable the following).
	//config->pwmTempVoltageThreshold            = 0.3090;
	//config->pwmTempVoltageThresholdDown        = 0.2655;

	config->minTxPower                         = -20;

	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

void asACR01B10B(boards_config_t* config) {
	// The ACR01B10B is similar to ACR01B10D, but lacks the dimmer enable pin.
	asACR01B10D(config);
	config->flags.usesNfcPins                  = false;
	config->pinEnableDimmer                    = PIN_NONE;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

//	// With patch 2:
//	config->pinEnableDimmer                    = 15;
//	config->flags.canDimOnWarmBoot             = true; // TODO: can it?
//	config->flags.dimmerOnWhenPinsFloat        = false;
}

/* ********************************************************************************************************************
 * Crownstone Built-in Two
 * *******************************************************************************************************************/

/**
 * The built-in two has an nRF52833 chipset. The pin layout however depends on the actual form factor being used,
 * for example the aQFN73, the QFN48 or the WLCSP assignments are different.
 */
void asACR01B13B(boards_config_t* config) {
	config->pinDimmer                          = 15;
	config->pinEnableDimmer                    = PIN_NONE;
	config->pinRelayOn                         = 17;
	config->pinRelayOff                        = 18; // P0.18 / RESET

	config->pinAinCurrent[GAIN_LOW]            = GpioToAin(3);
	config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(2);

	config->pinAinVoltage[GAIN_LOW]            = GpioToAin(31);
	config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(28);
	
	config->pinAinVoltageAfterLoad[GAIN_LOW]   = GpioToAin(30);
	config->pinAinVoltageAfterLoad[GAIN_HIGH]  = GpioToAin(29);

	config->pinAinZeroRef                      = GpioToAin(4); // REF
	config->pinAinDimmerTemp                   = GpioToAin(5);

	config->pinCurrentZeroCrossing             = GpioToAin(11);
	config->pinVoltageZeroCrossing             = GpioToAin(41); // P1.09

	config->pinRx                              = 10;
	config->pinTx                              = 9;
	
	config->pinGpio[0]                         = 24;  // GPIO P0.24 / AD20
	config->pinGpio[1]                         = 32;  // GPIO P1.00 / AD22
	config->pinGpio[2]                         = 34;  // GPIO P1.02 / W24
	config->pinGpio[3]                         = 36;  // GPIO P1.04 / U24

	config->flags.usesNfcPins                  = true;
	config->flags.dimmerInverted               = true;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.canTryDimmingOnBoot          = true;
	config->flags.canDimOnWarmBoot             = true;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN_TWO;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageRange                       = 1800;
	config->currentRange                       = 1800;

	// TODO: All the following values have to be calculated still, now set to some guessed values.
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

	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

/**
 * This prototype is based on the nRF52840 (in contrast with the unit above which is based on the nRF52833).
 */
void asACR01B15A(boards_config_t* config) {
	config->pinDimmer                          = 45;  // GPIO P1.13 / Gate-N
	config->pinEnableDimmer                    = 46;  // GPIO P1.14 / Gate-P

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
	
	config->pinCurrentZeroCrossing             = GpioToAin(8);
	config->pinVoltageZeroCrossing             = GpioToAin(41); // P1.09

	config->pinRx                              = 20;
	config->pinTx                              = 22;
	
	config->pinGpio[0]                         = 24;
	config->pinGpio[1]                         = 32;  // GPIO P1.00 / AD22
	config->pinGpio[2]                         = 34;  // GPIO P1.02 / W24
	config->pinGpio[3]                         = 36;  // GPIO P1.04 / U24
	config->pinGpio[4]                         = 38;  // GPIO P1.06
	config->pinGpio[5]                         = 39;  // GPIO P1.07
	config->pinGpio[6]                         = 9;
	config->pinGpio[7]                         = 16;
	config->pinGpio[8]                         = 21;
	config->pinGpio[9]                         = 47;  // GPIO P1.15

	config->flags.dimmerInverted               = true;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
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

	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

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
	config->flags.enableLeds                   = true;
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
	config->voltageZero[GAIN_SINGLE]           = 2003;
	// TODO: 1991 seems to be better than 1997 (check this old remark with theory).
	config->currentZero[GAIN_SINGLE]           = 1997;

	config->powerZero                          = 1500;

	// ADC values [0, 4095] map to [0V, 1.2V].
	config->voltageRange                       = 1200;
	config->currentRange                       = 1200;

	// About 1.5kOhm --> 90-100C
	config->pwmTempVoltageThreshold            = 0.76;
	// About 0.7kOhm --> 70-95C
	config->pwmTempVoltageThresholdDown        = 0.41;

	config->minTxPower                         = -20;

	config->scanIntervalUs                     = 140 * 1000;
	// This board cannot provide enough power for 100% scanning.
	config->scanWindowUs                       = 105 * 1000;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

/**
 * The second type of plug that is delivered to Crownstone users.
 *
 * Voltage measurements:
 *   - This plug has an offset as additional input for the ADC. It can do differential measurements.
 *   - The ADC is configured with a range of -1.2V to 1.2V.
 *   - Note, voltageRange=1200 is a bit of misnomer here, the complete range is 2x1200 mV = 2400 mV.
 *   - It means that a gain of 1/2 is used.
 *   - The voltage divider (R12/R13 also in this circuit) centers at 3.3V/2 by two identical 68k resistors.
 *   - The equivalent parallel resistor is 34k.
 *   - The opamp gain becomes 34k/10.3M = 0.00330 (or 302.94x when inverted).
 *   - If we divide 2.4V over 4096-1 we have 0.58608 mV per interval.
 *   - The voltage multiplier of 0.00058608 x 302.94 = 0.17754.
 *   - If the reference voltage is exactly 3.3V/2, we should have zero offset for our voltage.
 *   - If VDD is not exactly 3.3V, this is exactly the same for the voltage divider at the opamp as well.
 *   - Resistor values with accuracies of 1% are used. This can lead to a difference of 33 mV (calculation not shown).
 *   - This would amount to a maximum offset error of 33 / 0.58608 = 56 in ADC values.
 *
 * Temperature:
 *   - Threshold of 0.70 corresponds to about 50°C.
 *   - Threshold of 0.25 corresponds to about 90°C.
 *
 * TODO: Here voltageMultiplier=0.171f is used instead of 0.17754f. Check.
 * TODO: Here voltageZero=-99 is used instead of 0. The offset should be at max 56. Check.
 * TODO: The values used now might be empirical. Replace by theoretic values or tell why the calcluation is incorrect.
 */
void asACR01B2G(boards_config_t* config) {
	config->pinDimmer                          = 8;
	config->pinRelayOn                         = 6;
	config->pinRelayOff                        = 7;

	config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);

	config->pinAinZeroRef                      = GpioToAin(2);
	config->pinAinDimmerTemp                   = GpioToAin(5);

	config->pinRx                              = 20;
	config->pinTx                              = 19;

	config->pinLed[LED_RED]                    = 10;
	config->pinLed[LED_GREEN]                  = 9;

	config->flags.dimmerInverted               = false;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = true;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.usesNfcPins                  = false; // Set to true if you want to use the LEDs.
	config->flags.canTryDimmingOnBoot          = false;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	// The following two values are empirically determined, through calibration over 10 production crownstones
	config->voltageMultiplier[GAIN_SINGLE]     = 0.171f;
	config->currentMultiplier[GAIN_SINGLE]     = 0.00385f;

	// Calibrated by noisy data from 1 crownstone
	// The following two values are empirically determined, through calibration over noisy data from 1 Crownstone.
	config->voltageZero[GAIN_SINGLE]           = -99;
	config->currentZero[GAIN_SINGLE]           = -270;

	// The following value is empirically determined, through calibration over 10 production crownstones
	config->powerZero                          = 9000;

	// ADC values [-2048, 2047] map to [REF - 1.2V, REF + 1.2V].
	config->voltageRange                       = 1200;

	// ADC values [-2048, 2047] map to [REF - 0.6V, REF + 0.6V].
	config->currentRange                       = 600;

	config->pwmTempVoltageThreshold            = 0.70;
	config->pwmTempVoltageThresholdDown        = 0.25;

	config->minTxPower                         = -20;

	config->scanIntervalUs                     = 140 * 1000;

	// This board cannot provide enough power for 100% scanning.
	config->scanWindowUs                       = 105 * 1000;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

/* ********************************************************************************************************************
 * Crownstone Plug One
 * *******************************************************************************************************************/

/*
 * This plug has not been delivered to customers.
 *
 * This design has an improved power design. It has two opamps for both voltage and current.
 *
 * Remarks:
 *   - Make sure that pin P0.19 is floating (not listed here). It is a short to another pin (P0.30).
 *   - The pin for the dimmer is accidentally connected to a non-connected pin (A18).
 *   - It has been patched to pin LED red on pin 8.
 *
 * Temperature is set according to:
 *   T_0    = 25 (room temperature)
 *   R_0    = 10000 (from datasheet, 10k at 25 degrees Celcius)
 *   B_ntc  = 3434 K (from datasheet of NCP15XH103J03RC at 25-85 degrees Celcius)
 *   B_ntc  = 3380 K (if we assume it to stay cold at for 25-50 degrees Celcius)
 *   B_ntc  = 3455 K (if we assume it to become hot at 25-100 degrees Celcius)
 *   R_22   = 15000
 *   R_ntc  = R_0 * exp(B_ntc * (1/(T+273.15) - 1/(T_0+273.15)))
 *   V_temp = 3.3 * R_ntc/(R_22+R_ntc)
 *
 * We want to trigger between 76 < T < 82 degrees Celcius.
 *
 *   |  T | R_ntc | V_temp | B_ntc |
 *   | -- | ----  | ------ | ----- |
 *   | 25 | 10000 | 1.32   |       |
 *   | 76 |  1859 | 0.3639 |       | <- used value
 *   | 82 |  1575 | 0.3135 |       | <- used value
 *   | 90 |  1256 | 0.2551 |  3455 |
 *
 * Hence, 0.3135 < V_temp < 0.3639 in an inverted fashion (higher voltage means cooler).
 *
 * The voltage range with R10 = 36.5k and R6 = 10M (C9 neglectable) is 0.0365/(0.0365+10)*680 = 2.47V.
 * Thus it is 1.24V each way. A range of 1300 would be fine. Then 1240 would correspond to 240V RMS.
 * Hence, the voltage multiplier (from ADC values to volts RMS) is 0.19355.
 */
void asACR01B11A(boards_config_t* config) {
	config->pinDimmer                          = 8;
	config->pinRelayOn                         = 15;
	config->pinRelayOff                        = 13;

	config->pinAinCurrent[GAIN_LOW]            = GpioToAin(2);
	config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(3);

	config->pinAinVoltage[GAIN_LOW]            = GpioToAin(31);
	config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(29);

	config->pinAinDimmerTemp                   = GpioToAin(4);
	config->pinAinZeroRef                      = GpioToAin(5);

	config->pinRx                              = 22;
	config->pinTx                              = 20;

	config->pinGpio[0]                         = 24;
	config->pinGpio[1]                         = 32;
	config->pinGpio[2]                         = 34;
	config->pinGpio[3]                         = 36;

	config->flags.dimmerInverted               = true;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;
	config->flags.ledInverted                  = false;
	config->flags.dimmerTempInverted           = true;
	config->flags.usesNfcPins                  = false;
	config->flags.canTryDimmingOnBoot          = false;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

	config->deviceType                         = DEVICE_CROWNSTONE_PLUG_ONE;

	// All the values below are just copied from configuration values from other hardware and should be adjusted.
	config->voltageMultiplier[GAIN_SINGLE]     = 0.19355f;
	config->currentMultiplier[GAIN_SINGLE]     = 0.00385f;
	config->voltageZero[GAIN_SINGLE]           = 0;

	config->currentZero[GAIN_LOW]              = -270;
	config->currentZero[GAIN_HIGH]             = -270;

	config->powerZero                          = 9000;

	// ADC values [-2048, 2047] map to [REF - 1.8V, REF + 1.8V].
	config->voltageRange                       = 1800;

	// ADC values [-2048, 2047] map to [REF - 0.6V, REF + 0.6V].
	config->currentRange                       = 600;

	config->pwmTempVoltageThreshold            = 0.3639;
	config->pwmTempVoltageThresholdDown        = 0.3135;

	config->minTxPower                         = -20;

	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = config->scanIntervalUs;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

/* ********************************************************************************************************************
 * Dev Boards
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
	config->pinDimmer                          = 17;
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
	
	config->pinLed[LED2]                       = 19;
	config->pinLed[LED3]                       = 20;

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
	config->pinLed[3]                          = 20;

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
	config->voltageZero[GAIN_SINGLE]           = 1000;
	config->currentZero[GAIN_SINGLE]           = 1000;
	config->powerZero                          = 0;

	// ADC values [0, 4095] map to [0, 3.6V].
	config->voltageRange                       = 3600;
	config->currentRange                       = 3600;

	config->pwmTempVoltageThreshold            = 2.0;
	config->pwmTempVoltageThresholdDown        = 1.0;

	config->minTxPower                         = -40;

	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = 140 * 1000;
	config->tapToToggleDefaultRssiThreshold    = -35;
}

void asUsbDongle(boards_config_t* config) {
	asPca10040(config);
	config->deviceType                         = DEVICE_CROWNSTONE_USB;
}

/**
 * The Guidestone has pads for pin 9, 10, 25, 26, 27, SWDIO, SWDCLK, GND, VDD.
 */
void asGuidestone(boards_config_t* config) {
	config->pinRx                              = 25;
	config->pinTx                              = 26;

	config->flags.usesNfcPins                  = false;
	config->flags.dimmerInverted               = false;
	config->flags.enableUart                   = false;
	config->flags.enableLeds                   = false;

	config->deviceType                         = DEVICE_GUIDESTONE;

	config->minTxPower                         = -20;

	config->scanIntervalUs                     = 140 * 1000;
	config->scanWindowUs                       = 140 * 1000;
}

uint32_t configure_board(boards_config_t* config) {

	uint32_t hardwareBoard = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	if (hardwareBoard == 0xFFFFFFFF) {
		hardwareBoard = g_DEFAULT_HARDWARE_BOARD;
	}

	init(config);

	switch(hardwareBoard) {
		case ACR01B1A:
		case ACR01B1B:
		case ACR01B1C:
		case ACR01B1D:
		case ACR01B1E:
			asACR01B1D(config);
			break;

		case ACR01B10B:
			asACR01B10B(config);
			break;
		case ACR01B10D:
			asACR01B10D(config);
			break;

		case ACR01B13B:
			asACR01B13B(config);
			break;

		case ACR01B15A:
			asACR01B15A(config);
			break;

		case ACR01B2A:
		case ACR01B2B:
		case ACR01B2C:
			asACR01B2C(config);
			break;

		case ACR01B2E:
		case ACR01B2G:
			asACR01B2G(config);
			break;
		case ACR01B11A:
			asACR01B11A(config);
			break;
		case GUIDESTONE:
			asGuidestone(config);
			break;

		case PCA10036:
		case PCA10040:
		case PCA10056:
		case PCA10100:
			asPca10040(config);
			break;
		case CS_USB_DONGLE:
			asUsbDongle(config);
			break;

		default:
			// undefined board layout !!!
			asACR01B2C(config);
			return NRF_ERROR_INVALID_PARAM;
	}

	config->hardwareBoard = hardwareBoard;

	return NRF_SUCCESS;
}
