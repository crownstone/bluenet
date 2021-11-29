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
void init(boards_config_t* p_config) {
	for (uint8_t i = 0; i < GAIN_COUNT; ++i) {
		p_config->pinAinVoltage[i] = GAIN_UNUSED;
		p_config->pinAinCurrent[i] = GAIN_UNUSED;
		p_config->pinAinVoltageAfterLoad[i] = GAIN_UNUSED;
		p_config->voltageMultiplier[i] = 0.0;
		p_config->voltageAfterLoadMultiplier[i] = 0.0;
		p_config->currentMultiplier[i] = 0.0;
		p_config->voltageZero[i] = 0;
		p_config->voltageAfterLoadZero[i] = 0;
		p_config->currentZero[i] = 0;
	}
	for (uint8_t i = 0; i < GPIO_INDEX_COUNT; ++i) {
		p_config->pinGpio[i] = GPIO_UNUSED;
	}
	for (uint8_t i = 0; i < LED_COUNT; ++i) {
		p_config->pinLed[i] = LED_UNUSED;
	}
	for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
		p_config->pinButton[i] = BUTTON_UNUSED;
	}
	p_config->pinAinZeroRef                      = PIN_UNUSED;

	p_config->pinGpioEnablePwm                   = PIN_UNUSED;
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
			return PIN_UNUSED;
		}
	default:
		return PIN_UNUSED;
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
void asACR01B1D(boards_config_t* p_config) {
	// Pins can be set either to XXX_UNUSED or this pin
	p_config->pinUnused                          = 11;

	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioEnablePwm                   = p_config->pinUnused;

	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;

	p_config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	p_config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);
	p_config->pinAinPwmTemp                      = GpioToAin(5);
	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;
	p_config->pinLed[LED_RED]                    = 10;
	p_config->pinLed[LED_GREEN]                  = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = false;

	p_config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN;

	// TODO: Explain this value
	p_config->voltageMultiplier[GAIN_SINGLE]     = 0.2f;
	
	// TODO: Explain this value
	p_config->currentMultiplier[GAIN_SINGLE]     = 0.0044f;
	
	// TODO: Explain this value
	p_config->voltageZero[GAIN_SINGLE]           = 1993;
	
	// TODO: Explain this value
	p_config->currentZero[GAIN_SINGLE]           = 1980;

	// TODO: Explain this value
	p_config->powerZero                          = 3500;

	p_config->voltageRange                       = 1200;
	p_config->currentRange                       = 1200;

	// TODO: Explain these comments. About 1.5kOhm --> 90-100C
	p_config->pwmTempVoltageThreshold            = 0.76;
	// TODO: Explain these comments. About 0.7kOhm --> 70-95C
	p_config->pwmTempVoltageThresholdDown        = 0.41;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	// This board cannot provide enough power for 100% scanning!
	p_config->scanWindowUs                       = 105 * 1000;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
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
void asACR01B10D(boards_config_t* p_config) {
	// not a particular pin assigned to be dangling
	p_config->pinUnused                          = PIN_UNUSED;

	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioEnablePwm                   = 10;
	p_config->pinGpioRelayOn                     = 14;
	p_config->pinGpioRelayOff                    = 13;

	p_config->pinAinCurrent[GAIN_LOW]            = GpioToAin(30);
	p_config->pinAinCurrent[GAIN_MIDDLE]         = GpioToAin(29);
	p_config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(28);

	p_config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);

	p_config->pinAinZeroRef                      = GpioToAin(2);
	p_config->pinAinPwmTemp                      = GpioToAin(4);

	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = false;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN_ONE;

	// TODO: Explain why not -0.26632 (see above). If incorrect, replace.
	p_config->voltageMultiplier[GAIN_SINGLE]     = -0.2547;

	// TODO: Explain this value
	p_config->currentMultiplier[GAIN_LOW]        = 0.01486f;

	// TODO: Calculate the following values (now set to something arbitrary)
	p_config->currentMultiplier[GAIN_MIDDLE]     = 0.015;
	p_config->currentMultiplier[GAIN_HIGH]       = 0.015;

	// TODO: Explain why not 512 (see above). If 500 incorrect, replace.
	p_config->voltageZero[GAIN_SINGLE]           = 500;

	// TODO: Explain this value
	p_config->currentZero[GAIN_LOW]              = -125;

	// TODO: Calculate the following values (now set to something arbitrary)
	p_config->currentZero[GAIN_MIDDLE]           = -125;
	p_config->currentZero[GAIN_HIGH]             = -125;

	// TODO: Explain this value
	p_config->powerZero                          = 800;

	p_config->voltageRange                       = 1800;
	p_config->currentRange                       = 1200;

	// TODO: These are incorrectly calculated with R_12 = 16k and B_ntc = 3380 K.
	p_config->pwmTempVoltageThreshold            = 0.35; // 0.3518
	p_config->pwmTempVoltageThresholdDown        = 0.30; // 0.3036
	
	// TODO: It should be with R_12 = 18k and B_ntc = 3434 K (enable the following).
	//p_config->pwmTempVoltageThreshold            = 0.3090;
	//p_config->pwmTempVoltageThresholdDown        = 0.2655;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	p_config->scanWindowUs                       = p_config->scanIntervalUs;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
}

/* ********************************************************************************************************************
 * Crownstone Built-in Two
 * *******************************************************************************************************************/

/**
 * The built-in two has an nRF52833 chipset. The pin layout however depends on the actual form factor being used,
 * for example the aQFN73, the QFN48 or the WLCSP assignments are different.
 */
void asACR01B13B(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 15;

	p_config->pinGpioRelayOn                     = 17;
	p_config->pinGpioRelayOff                    = 18; // P0.18 / RESET

	p_config->pinAinCurrent[GAIN_LOW]            = GpioToAin(3);
	p_config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(2);

	p_config->pinAinVoltage[GAIN_LOW]            = GpioToAin(31);
	p_config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(28);
	
	p_config->pinAinVoltageAfterLoad[GAIN_LOW]   = GpioToAin(30);
	p_config->pinAinVoltageAfterLoad[GAIN_HIGH]  = GpioToAin(29);

	p_config->pinAinZeroRef                      = GpioToAin(4); // REF
	p_config->pinAinPwmTemp                      = GpioToAin(5);

	p_config->pinAinCurrentZeroCrossing          = GpioToAin(11);
	p_config->pinAinVoltageZeroCrossing          = GpioToAin(41); // P1.09

	p_config->pinGpioRx                          = 10;
	p_config->pinGpioTx                          = 9;
	
	p_config->pinGpio[0]                         = 24;  // GPIO P0.24 / AD20
	p_config->pinGpio[1]                         = 32;  // GPIO P1.00 / AD22
	p_config->pinGpio[2]                         = 34;  // GPIO P1.02 / W24
	p_config->pinGpio[3]                         = 36;  // GPIO P1.04 / U24

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = true;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = false;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN_TWO;

	p_config->voltageRange                       = 1800;
	p_config->currentRange                       = 1800;

	// TODO: All the following values have to be calculated still, now set to some guessed values.
	p_config->voltageMultiplier[GAIN_LOW]        = -0.25;
	p_config->voltageMultiplier[GAIN_HIGH]       = -0.25;
	
	p_config->voltageAfterLoadMultiplier[GAIN_LOW]  = -0.25;
	p_config->voltageAfterLoadMultiplier[GAIN_HIGH] = -0.25;

	p_config->currentMultiplier[GAIN_LOW]        = 0.01;
	p_config->currentMultiplier[GAIN_HIGH]       = 0.01;

	p_config->voltageZero[GAIN_LOW]              = 0;
	p_config->voltageZero[GAIN_HIGH]             = 0;

	p_config->currentZero[GAIN_LOW]              = 0;
	p_config->currentZero[GAIN_HIGH]             = 0;

	p_config->powerZero                          = 0;

	p_config->pwmTempVoltageThreshold            = 0.2;
	p_config->pwmTempVoltageThresholdDown        = 0.18;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	p_config->scanWindowUs                       = p_config->scanIntervalUs;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
}

/**
 * This prototype is based on the nRF52840 (in contrast with the unit above which is based on the nRF52833).
 */
void asACR01B15A(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 45;  // GPIO P1.13 / Gate-N
	p_config->pinGpioEnablePwm                   = 46;  // GPIO P1.14 / Gate-P

	p_config->pinGpioRelayOn                     = 13;
	p_config->pinGpioRelayOff                    = 15;

	p_config->pinAinCurrent[GAIN_LOW]            = GpioToAin(2);
	p_config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(30);

	p_config->pinAinVoltage[GAIN_LOW]            = GpioToAin(28);
	p_config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(3);
	
	p_config->pinAinVoltageAfterLoad[GAIN_LOW]   = GpioToAin(29);
	p_config->pinAinVoltageAfterLoad[GAIN_HIGH]  = GpioToAin(31);

	p_config->pinAinZeroRef                      = GpioToAin(5);
	p_config->pinAinPwmTemp                      = GpioToAin(4);
	
	p_config->pinAinCurrentZeroCrossing          = GpioToAin(8);
	p_config->pinAinVoltageZeroCrossing          = GpioToAin(41); // P1.09

	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 22;
	
	p_config->pinGpio[0]                         = 24;
	p_config->pinGpio[1]                         = 32;  // GPIO P1.00 / AD22
	p_config->pinGpio[2]                         = 34;  // GPIO P1.02 / W24
	p_config->pinGpio[3]                         = 36;  // GPIO P1.04 / U24
	p_config->pinGpio[4]                         = 38;  // GPIO P1.06
	p_config->pinGpio[5]                         = 39;  // GPIO P1.07
	p_config->pinGpio[6]                         = 9;
	p_config->pinGpio[7]                         = 16;
	p_config->pinGpio[8]                         = 21;
	p_config->pinGpio[9]                         = 47;  // GPIO P1.15

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = true;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = false;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = DEVICE_CROWNSTONE_BUILTIN_TWO;

	p_config->voltageRange                       = 1800;
	p_config->currentRange                       = 1800;

	//TODO: All the following values have to be calculated still, now set to some guessed values.
	p_config->voltageMultiplier[GAIN_LOW]        = -0.25;
	p_config->voltageMultiplier[GAIN_HIGH]       = -0.25;
	
	p_config->voltageAfterLoadMultiplier[GAIN_LOW]  = -0.25;
	p_config->voltageAfterLoadMultiplier[GAIN_HIGH] = -0.25;

	p_config->currentMultiplier[GAIN_LOW]        = 0.01;
	p_config->currentMultiplier[GAIN_HIGH]       = 0.01;

	p_config->voltageZero[GAIN_LOW]              = 0;
	p_config->voltageZero[GAIN_HIGH]             = 0;

	p_config->currentZero[GAIN_LOW]              = 0;
	p_config->currentZero[GAIN_HIGH]             = 0;

	p_config->powerZero                          = 0;

	p_config->pwmTempVoltageThreshold            = 0.2;
	p_config->pwmTempVoltageThresholdDown        = 0.18;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	p_config->scanWindowUs                       = p_config->scanIntervalUs;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
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
void asACR01B2C(boards_config_t* p_config) {
	// Pins can be set either to XXX_UNUSED or this pin
	p_config->pinUnused                          = 11;

	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioEnablePwm                   = p_config->pinUnused;
	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;
	p_config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	p_config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);
	p_config->pinAinPwmTemp                      = GpioToAin(5);
	p_config->pinAinZeroRef                      = PIN_UNUSED;
	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;

	p_config->pinLed[LED_RED]                    = 10;
	p_config->pinLed[LED_GREEN]                  = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = false;

	p_config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	p_config->voltageMultiplier[GAIN_SINGLE]     = 0.2f;
	p_config->currentMultiplier[GAIN_SINGLE]     = 0.0045f;
	
	// TODO: 2010 seems to be better than 2003 (check this old remark with theory).
	p_config->voltageZero[GAIN_SINGLE]           = 2003;
	// TODO: 1991 seems to be better than 1997 (check this old remark with theory).
	p_config->currentZero[GAIN_SINGLE]           = 1997;

	p_config->powerZero                          = 1500;
	p_config->voltageRange                       = 1200;
	p_config->currentRange                       = 1200;

	// About 1.5kOhm --> 90-100C
	p_config->pwmTempVoltageThreshold            = 0.76;
	// About 0.7kOhm --> 70-95C
	p_config->pwmTempVoltageThresholdDown        = 0.41;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	// This board cannot provide enough power for 100% scanning.
	p_config->scanWindowUs                       = 105 * 1000;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
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
void asACR01B2G(boards_config_t* p_config) {
	// Pins can be set either to XXX_UNUSED or this pin
	p_config->pinUnused                          = 11;
	
	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;

	p_config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(3);
	p_config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(4);

	p_config->pinAinZeroRef                      = GpioToAin(2);
	p_config->pinAinPwmTemp                      = GpioToAin(5);

	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;

	p_config->pinLed[LED_RED]                    = 10;
	p_config->pinLed[LED_GREEN]                  = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	// The following two values are empirically determined, through calibration over 10 production crownstones
	p_config->voltageMultiplier[GAIN_SINGLE]     = 0.171f;
	p_config->currentMultiplier[GAIN_SINGLE]     = 0.00385f;

	// Calibrated by noisy data from 1 crownstone
	// The following two values are empirically determined, through calibration over noisy data from 1 Crownstone.
	p_config->voltageZero[GAIN_SINGLE]           = -99;
	p_config->currentZero[GAIN_SINGLE]           = -270;

	// The following value is empirically determined, through calibration over 10 production crownstones
	p_config->powerZero                          = 9000;

	p_config->voltageRange                       = 1200;
	p_config->currentRange                       = 600;

	p_config->pwmTempVoltageThreshold            = 0.70;
	p_config->pwmTempVoltageThresholdDown        = 0.25;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;

	// This board cannot provide enough power for 100% scanning.
	p_config->scanWindowUs                       = 105 * 1000;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
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
void asACR01B11A(boards_config_t* p_config) {
	// Pins can be set either to XXX_UNUSED or this pin
	p_config->pinUnused                          = 11;

	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioEnablePwm                   = p_config->pinUnused;
	p_config->pinGpioRelayOn                     = 15;
	p_config->pinGpioRelayOff                    = 13;

	p_config->pinAinCurrent[GAIN_LOW]            = GpioToAin(2);
	p_config->pinAinCurrent[GAIN_HIGH]           = GpioToAin(3);

	p_config->pinAinVoltage[GAIN_LOW]            = GpioToAin(31);
	p_config->pinAinVoltage[GAIN_HIGH]           = GpioToAin(29);

	p_config->pinAinPwmTemp                      = GpioToAin(4);
	p_config->pinAinZeroRef                      = GpioToAin(5);

	p_config->pinGpioRx                          = 22;
	p_config->pinGpioTx                          = 20;

	p_config->pinGpio[0]                         = 24;
	p_config->pinGpio[1]                         = 32;
	p_config->pinGpio[2]                         = 34;
	p_config->pinGpio[3]                         = 36;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = true;
	p_config->flags.hasSerial                    = true;
	p_config->flags.hasLed                       = false;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = DEVICE_CROWNSTONE_PLUG_ONE;

	// All the values below are just copied from configuration values from other hardware and should be adjusted.
	p_config->voltageMultiplier[GAIN_SINGLE]     = 0.19355f;
	p_config->currentMultiplier[GAIN_SINGLE]     = 0.00385f;
	p_config->voltageZero[GAIN_SINGLE]           = 0;

	p_config->currentZero[GAIN_LOW]              = -270;
	p_config->currentZero[GAIN_HIGH]             = -270;

	p_config->powerZero                          = 9000;

	p_config->voltageRange                       = 1800;
	p_config->currentRange                       = 600;

	p_config->pwmTempVoltageThreshold            = 0.3639;
	p_config->pwmTempVoltageThresholdDown        = 0.3135;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	p_config->scanWindowUs                       = p_config->scanIntervalUs;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
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
void asPca10040(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 17;
	p_config->pinGpioEnablePwm                   = 22;
	p_config->pinGpioRelayOn                     = 11;
	p_config->pinGpioRelayOff                    = 12;
	p_config->pinAinCurrent[GAIN_SINGLE]         = GpioToAin(3);
	p_config->pinAinVoltage[GAIN_SINGLE]         = GpioToAin(2);
	p_config->pinAinZeroRef                      = GpioToAin(28);
	p_config->pinAinPwmTemp                      = GpioToAin(2);
	p_config->pinGpioRx                          = 8;
	p_config->pinGpioTx                          = 6;
	
	p_config->pinLed[LED_RED]                    = 19;
	p_config->pinLed[LED_GREEN]                  = 20;

	p_config->pinGpio[0]                         = 27;  // Also SCL
	p_config->pinGpio[1]                         = 26;  // Also SDA
	p_config->pinGpio[2]                         = 25;
	p_config->pinGpio[3]                         = 24;

	p_config->pinButton[0]                       = 13;
	p_config->pinButton[1]                       = 14;
	p_config->pinButton[2]                       = 15;
	p_config->pinButton[3]                       = 16;
	
	p_config->pinLed[0]                          = 17;
	p_config->pinLed[1]                          = 18;
	p_config->pinLed[2]                          = 19;
	p_config->pinLed[3]                          = 20;

	p_config->flags.hasRelay                     = false;
	p_config->flags.pwmInverted                  = true;
	p_config->flags.hasSerial                    = true;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = true;
	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = false;

	p_config->deviceType                         = DEVICE_CROWNSTONE_PLUG;

	// All values below are set to something rather than nothing, but are not truly in use.
	p_config->voltageZero[GAIN_SINGLE]           = 1000;
	p_config->currentZero[GAIN_SINGLE]           = 1000;
	p_config->powerZero                          = 0;
	p_config->voltageRange                       = 3600;
	p_config->currentRange                       = 3600;

	p_config->pwmTempVoltageThreshold            = 2.0;
	p_config->pwmTempVoltageThresholdDown        = 1.0;

	p_config->minTxPower                         = -40;

	p_config->scanIntervalUs                     = 140 * 1000;
	p_config->scanWindowUs                       = 140 * 1000;
	p_config->tapToToggleDefaultRssiThreshold    = -35;
}

void asUsbDongle(boards_config_t* p_config) {
	asPca10040(p_config);
	p_config->deviceType                         = DEVICE_CROWNSTONE_USB;
}

/**
 * The Guidestone has pads for pin 9, 10, 25, 26, 27, SWDIO, SWDCLK, GND, VDD.
 */
void asGuidestone(boards_config_t* p_config) {
	p_config->pinGpioRx                          = 25;
	p_config->pinGpioTx                          = 26;

	p_config->flags.hasRelay                     = false;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = false;

	p_config->deviceType                         = DEVICE_GUIDESTONE;

	p_config->minTxPower                         = -20;

	p_config->scanIntervalUs                     = 140 * 1000;
	p_config->scanWindowUs                       = 140 * 1000;
}

uint32_t configure_board(boards_config_t* p_config) {

	uint32_t hardwareBoard = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	if (hardwareBoard == 0xFFFFFFFF) {
		hardwareBoard = g_DEFAULT_HARDWARE_BOARD;
	}

	init(p_config);

	switch(hardwareBoard) {
	case ACR01B1A:
	case ACR01B1B:
	case ACR01B1C:
	case ACR01B1D:
	case ACR01B1E:
		asACR01B1D(p_config);
		break;

	case ACR01B10B:
	case ACR01B10D:
		asACR01B10D(p_config);
		break;

	case ACR01B13B:
		asACR01B13B(p_config);
		break;
	
	case ACR01B15A:
		asACR01B15A(p_config);
		break;

	case ACR01B2A:
	case ACR01B2B:
	case ACR01B2C:
		asACR01B2C(p_config);
		break;

	case ACR01B2E:
	case ACR01B2G:
		asACR01B2G(p_config);
		break;
	case ACR01B11A:
		asACR01B11A(p_config);
		break;
	case GUIDESTONE:
		asGuidestone(p_config);
		break;

	case PCA10036:
	case PCA10040:
	case PCA10056:
	case PCA10100:
		asPca10040(p_config);
		break;
	case CS_USB_DONGLE:
		asUsbDongle(p_config);
		break;

	default:
		// undefined board layout !!!
		asACR01B2C(p_config);
		return NRF_ERROR_INVALID_PARAM;
	}
	p_config->hardwareBoard = hardwareBoard;

	return NRF_SUCCESS;
}
