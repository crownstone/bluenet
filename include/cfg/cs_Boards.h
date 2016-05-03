/*
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

/**
 * Convention:
 *   * use TABS for identation, but use SPACES for column-wise representations!
 */
#pragma once

//! Current accepted HARDWARE_BOARD types

#define PCA10001             0
#define NRF6310              1
#define CROWNSTONE           5
#define VIRTUALMEMO          7
#define CROWNSTONE2          8
#define CROWNSTONE_SENSOR    9
#define PCA10000             10
#define CROWNSTONE3          11
#define CROWNSTONE4          12
#define NORDIC_BEACON        13
#define DOBEACON             14
#define CROWNSTONE5          15

#ifndef HARDWARE_BOARD
#error "Add HARDWARE_BOARD=... to CMakeBuild.config"
#endif

#if(HARDWARE_BOARD==CROWNSTONE)

#define PIN_GPIO_SWITCH      3                   //! this is p0.03 or gpio 3
#define PIN_GPIO_RELAY_ON    0                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          6                   //! ain6 is p0.05 or gpio 5
//#define PIN_AIN_LPCOMP       5                   //! ain5 is p0.04 or gpio 4
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      6                   //! ain6 is gpio 5
#define PIN_AIN_VOLTAGE      5                   //! ain5 is gpio 4, actually also current sense
#define PIN_GPIO_RX          6                   //! this is p0.06 or gpio 6
#define PIN_GPIO_TX          1                   //! this is p0.01 or gpio 1

//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 120
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       1
#endif


#if(HARDWARE_BOARD==CROWNSTONE2)
//! v0.86
#define PIN_GPIO_SWITCH      3                   //! this is p0.03 or gpio 3
#define PIN_GPIO_RELAY_ON    5                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP       2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      2                   //! ain2 is gpio 1
#define PIN_AIN_VOLTAGE      6                   //! ain6 is gpio 5, not actually working
#define PIN_GPIO_RX          4                   //! this is p0.04 or gpio 4
#define PIN_GPIO_TX          2                   //! this is p0.02 or gpio 2

//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 20
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       5
#endif


#if(HARDWARE_BOARD==CROWNSTONE3)
//! v0.90
#define PIN_GPIO_SWITCH      3                   //! this is p0.03 or gpio 3
#define PIN_GPIO_RELAY_ON    5                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP       2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      2                   //! ain2 is gpio 1
#define PIN_AIN_VOLTAGE      7                   //! ain7 is gpio 6
#define PIN_GPIO_RX          2                   //! this is p0.02 or gpio 2
#define PIN_GPIO_TX          4                   //! this is p0.04 or gpio 4

//! Switch pin should be low to switch lights on
#define SWITCH_INVERSED
//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 1
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       80
#endif


#if(HARDWARE_BOARD==CROWNSTONE4)
//! v0.92
#define PIN_GPIO_SWITCH      4                   //! this is p0.04 or gpio 4
#define PIN_GPIO_RELAY_ON    5                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP       2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      2                   //! ain2 is p0.01 or gpio 1
#define PIN_AIN_VOLTAGE      7                   //! ain7 is p0.06 or gpio 6
#define PIN_GPIO_RX          2                   //! this is p0.02 or gpio 2
#define PIN_GPIO_TX          3                   //! this is p0.03 or gpio 3

//! Switch pin should be low to switch lights on
#define SWITCH_INVERSED
//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 1
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       80
#endif


#if(HARDWARE_BOARD==CROWNSTONE5)
//! plugin quant
#define PIN_GPIO_SWITCH      12                  //! p0.12
#define PIN_GPIO_RELAY_ON    11                  //! p0.11
#define PIN_GPIO_RELAY_OFF   10                  //! p0.10
#define PIN_AIN_CURRENT      7                   //! ain7 is p0.06
#define PIN_AIN_VOLTAGE      6                   //! ain6 is p0.05
#define PIN_GPIO_RX          15                  //! p0.15
#define PIN_GPIO_TX          16                  //! p0.16

//! Switch pin should be low to switch lights on
#define SWITCH_INVERSED
//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 1
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       80
#endif


#if(HARDWARE_BOARD==DOBEACON)
// v0.92
//#define PIN_GPIO_SWITCH      4                   // this is p0.04 or gpio 4
//#define PIN_GPIO_RELAY_ON    0                   // something unused
//#define PIN_GPIO_RELAY_OFF   0                   // something unused
//#define PIN_AIN_ADC          2                   // ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP       2                   // ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP_REF   0                   // ref0 is p0.00 or gpio 0
//#define PIN_AIN_CURRENT      0                   // ain2 is p0.01 or gpio 1
//#define PIN_AIN_VOLTAGE      0                   // ain7 is p0.06 or gpio 6
#define PIN_GPIO_RX          2                   // this is p0.02 or gpio 2
#define PIN_GPIO_TX          3                   // this is p0.03 or gpio 3

#define PIN_GPIO_LED_1       6					 // this is p0.07, GREEN
#define PIN_GPIO_LED_2       7					 // this is p0.07, RED
#define HAS_LEDS             1

// Switch pin should be low to switch lights on
//#define SWITCH_INVERSED
// resistance of the shunt in milli ohm
//#define SHUNT_VALUE                 1
// amplification of the voltage over the shunt, to the adc input of the chip
//#define VOLTAGE_AMPLIFICATION       80
#endif


#if(HARDWARE_BOARD==CROWNSTONE_SENSOR)

#define PIN_GPIO_SWITCH      3                   //! this is p0.03 or gpio 3
#define PIN_GPIO_RELAY_ON    5                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          6                   //! ain6 is p0.05 or gpio 5
//#define PIN_AIN_LPCOMP       5                   //! ain5 is p0.04 or gpio 4
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      6                   //! ain6 is p0.05 or gpio 5
#define PIN_AIN_VOLTAGE      5                   //! ain5 is p0.04 or gpio 4
#define PIN_GPIO_RX          6                   //! this is p0.06 or gpio 6
#define PIN_GPIO_TX          1                   //! this is p0.01 or gpio 1

#define PIN_AIN_SENSOR       3                   //! ain3 is p0.02 or gpio 2
#define PIN_GPIO_BUTTON      2

//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 120
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       1
#endif


#if(HARDWARE_BOARD==NRF6310)

#define PIN_GPIO_LED0        8                   //! this is p1.0 or gpio 8
#define PIN_GPIO_LED1        9                   //! this is p1.1 or gpio 9
#define PIN_GPIO_LED2        10
#define PIN_GPIO_LED3        11
#define PIN_GPIO_LED4        12
#define PIN_GPIO_LED5        13
#define PIN_GPIO_LED6        14
#define PIN_GPIO_LED7        15

#define PIN_GPIO_SWITCH      PIN_GPIO_LED1       //! show switch as LED
#define PIN_GPIO_RELAY_ON    0                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          2                   //! ain2 is p0.1 or gpio 1
//#define PIN_AIN_LPCOMP       3                   //! ain3 is p0.2 or gpio 2
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      2                   //! ain2 is p0.01 or gpio 1
#define PIN_AIN_VOLTAGE      3                   //! ain3 is p0.02 or gpio 2
#define PIN_GPIO_RX          16
#define PIN_GPIO_TX          17

//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 1
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       1
#endif


#if(HARDWARE_BOARD==PCA10001)

#define PIN_GPIO_LED0        18                  //! led
#define PIN_GPIO_LED1        19                  //! led

//#define PIN_GPIO_LED_CON     PIN_GPIO_LED1       //! shows connection state on the evaluation board
#define PIN_GPIO_SWITCH      PIN_GPIO_LED0       //! show switch as LED
#define PIN_GPIO_RELAY_ON    0                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          2                   //! ain2 is p0.01 or gpio 1
//#define PIN_AIN_LPCOMP       3                   //! ain3 is p0.02 or gpio 2
//#define PIN_AIN_LPCOMP_REF   0                   //! ref0 is p0.00 or gpio 0
#define PIN_AIN_CURRENT      2                   //! ain2 is p0.01 or gpio 1
#define PIN_AIN_VOLTAGE      3                   //! ain3 is p0.02 or gpio 2
#define PIN_GPIO_RX          11                  //! this is p0.11 or gpio 11
#define PIN_GPIO_TX          9                   //! this is p0.09 or gpio 9

#define PIN_AIN_SENSOR       4
#define BUTTON_0             16
#define BUTTON_1             17

//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 1
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       1
#endif


#if(HARDWARE_BOARD==PCA10000)

#define LED_RGB_RED          21
#define LED_RGB_GREEN        22
#define LED_RGB_BLUE         23

//#define PIN_GPIO_LED_CON     PIN_GPIO_GREEN       //! shows connection state on the evaluation board
#define PIN_GPIO_SWITCH      LED_RGB_BLUE        //! show switch as LED
#define PIN_GPIO_RELAY_ON    0                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused
//#define PIN_AIN_ADC          2                   //! something unused
//#define PIN_AIN_LPCOMP       2                   //! something unused
//#define PIN_AIN_LPCOMP_REF   0                   //! something unused
#define PIN_AIN_CURRENT      2                   //! something unused
#define PIN_AIN_VOLTAGE      2                   //! something unused
#define PIN_GPIO_RX          11                  //! this is p0.11 or gpio 11
#define PIN_GPIO_TX          9                   //! this is p0.09 or gpio 9

//! Switch pin should be low to switch lights on
#define SWITCH_INVERSED
//! resistance of the shunt in milli ohm
#define SHUNT_VALUE                 1
//! amplification of the voltage over the shunt, to the adc input of the chip
#define VOLTAGE_AMPLIFICATION       1
#endif


#if(HARDWARE_BOARD==VIRTUALMEMO)

#define PIN_GPIO_LED0        7                   //! this is p0.07 or gpio 7
#define PIN_GPIO_LED1        8                   //! this is p0.08 or gpio 8
#define PIN_GPIO_LED2        9
#define PIN_GPIO_LED3        10
#define PIN_GPIO_LED4        11
#define PIN_GPIO_LED5        12
#define PIN_GPIO_LED6        13
#define PIN_GPIO_LED7        14

#define PIN_GPIO_RX          15
#define PIN_GPIO_TX          16

#endif


#if(HARDWARE_BOARD==NORDIC_BEACON)

#define LED_RGB_RED          16                  //! this is p0.16 or gpio 16
#define LED_RGB_GREEN        12                  //! this is p0.12 or gpio 12
#define LED_RGB_BLUE         15                  //! this is p0.15 or gpio 15

#define PIN_GPIO_RX          9                   //! this is p0.09 or gpio 9
#define PIN_GPIO_TX          1                   //! this is p0.01 or gpio 1

//#define PIN_GPIO_LED_CON     LED_RGB_GREEN       //! shows connection state on the evaluation board
#define PIN_GPIO_SWITCH      LED_RGB_BLUE        //! show switch as LED
#define PIN_GPIO_RELAY_ON    0                   //! something unused
#define PIN_GPIO_RELAY_OFF   0                   //! something unused

#define PIN_GPIO_BUTTON      20                  //! this is p0.20 or gpio 20

#define SWITCH_INVERSED

#endif

//! Sanity checks

#ifndef PIN_GPIO_RX
#error "For UART, PIN_RX must be defined"
#endif

#ifndef PIN_GPIO_TX
#error "For UART, PIN_TX must be defined"
#endif

#ifndef HARDWARE_VERSION
#error "You have to specify the hardware version of your chip"
#endif
