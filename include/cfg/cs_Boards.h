/**
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

// Current accepted BOARD types

#define PCA10001             0
#define NRF6310              1
#define NRF51822_DONGLE      2
#define NRF51822_EVKIT       3
#define RFDUINO              4
#define CROWNSTONE           5
#define NRF51422             6
#define VIRTUALMEMO          7
#define CROWNSTONE2          8
#define CROWNSTONE_SENSOR    9
#define PCA10000             10

#ifndef BOARD
#error "Add BOARD=... to CMakeBuild.config"
#endif

#if(BOARD==RFDUINO)

#define PIN_GPIO_LED0        2                   // gpio 2
#define PIN_GPIO_LED1        3                   // gpio 3

#define PIN_RED              2                   // this is gpio 2 (bottom pin)
#define PIN_GREEN            3                   // this is gpio 3 (second pin)
#define PIN_BLUE             4                   // this is gpio 4 (third pin)

#endif


#if(BOARD==NRF6310)

#define PIN_GPIO_LED0        8                   // this is p1.0 or gpio 8
#define PIN_GPIO_LED1        9                   // this is p1.1 or gpio 9
#define PIN_GPIO_LED2        10
#define PIN_GPIO_LED3        11
#define PIN_GPIO_LED4        12
#define PIN_GPIO_LED5        13
#define PIN_GPIO_LED6        14
#define PIN_GPIO_LED7        15

#define PIN_GPIO_SWITCH      PIN_GPIO_LED1       // just show switch as LED
#define PIN_AIN_ADC          2                   // ain2 is p0.1 or gpio 1
#define PIN_GPIO_RX          16
#define PIN_GPIO_TX          17
#define PIN_AIN_LPCOMP       3                   // ain3 is p0.2 or gpio 2

#endif


#if(BOARD==CROWNSTONE)

#define PIN_GPIO_LED0        8                   // something we don't use!
#define PIN_GPIO_LED1        8                   // something we don't use!
#define PIN_GPIO_LED2        8                   // something we don't use!
#define PIN_GPIO_LED3        8                   // something we don't use!
#define PIN_GPIO_LED4        8                   // something we don't use!
#define PIN_GPIO_LED5        8                   // something we don't use!
#define PIN_GPIO_LED6        8                   // something we don't use!
#define PIN_GPIO_LED7        8                   // something we don't use!

#define PIN_GPIO_SWITCH      3                   // this is p0.03 or gpio 3
//#define PIN_AIN_ADC          5                   // ain5 is p0.04 or gpio 4
//#define PIN_AIN_LPCOMP       6                   // ain6 is p0.05 or gpio 5
#define PIN_AIN_ADC          6                   // ain6 is p0.05 or gpio 5
#define PIN_AIN_LPCOMP       5                   // ain5 is p0.04 or gpio 4

#define PIN_AIN_LPCOMP_REF   0                   // ref0 is p0.00 or gpio 0
#define PIN_GPIO_RX          6                   // this is p0.06 or gpio 6
#define PIN_GPIO_TX          1                   // this is p0.01 or gpio 1

#endif


#if(BOARD==CROWNSTONE2)

#define PIN_GPIO_LED0        8                   // something we don't use!
#define PIN_GPIO_LED1        8                   // something we don't use!
#define PIN_GPIO_LED2        8                   // something we don't use!
#define PIN_GPIO_LED3        8                   // something we don't use!
#define PIN_GPIO_LED4        8                   // something we don't use!
#define PIN_GPIO_LED5        8                   // something we don't use!
#define PIN_GPIO_LED6        8                   // something we don't use!
#define PIN_GPIO_LED7        8                   // something we don't use!

#define PIN_GPIO_SWITCH      3                   // this is p0.03 or gpio 3
#define PIN_AIN_ADC          2                   // ain2 is p0.01 or gpio 1
#define PIN_AIN_LPCOMP       2                   // ain2 is p0.01 or gpio 1

#define PIN_AIN_LPCOMP_REF   0                   // ref0 is p0.00 or gpio 0
#define PIN_GPIO_RX          4                   // this is p0.04 or gpio 4
#define PIN_GPIO_TX          2                   // this is p0.02 or gpio 2

#endif

#if(BOARD==CROWNSTONE_SENSOR)

#define PIN_GPIO_LED0        8                   // something we don't use!
#define PIN_GPIO_LED1        8                   // something we don't use!
#define PIN_GPIO_LED2        8                   // something we don't use!
#define PIN_GPIO_LED3        8                   // something we don't use!
#define PIN_GPIO_LED4        8                   // something we don't use!
#define PIN_GPIO_LED5        8                   // something we don't use!
#define PIN_GPIO_LED6        8                   // something we don't use!
#define PIN_GPIO_LED7        8                   // something we don't use!

#define PIN_GPIO_SWITCH      3                   // this is p0.03 or gpio 3

#define PIN_AIN_ADC          6                   // ain6 is p0.05 or gpio 5
#define PIN_AIN_LPCOMP       5                   // ain5 is p0.04 or gpio 4
#define PIN_AIN_SENSOR       3                   // ain3 is p0.02 or gpio 2

#define PIN_GPIO_BUTTON      2

#define PIN_AIN_LPCOMP_REF   0                   // ref0 is p0.00 or gpio 0
#define PIN_GPIO_RX          6                   // this is p0.06 or gpio 6
#define PIN_GPIO_TX          1                   // this is p0.01 or gpio 1

#endif


#if(BOARD==PCA10001)

#define PIN_GPIO_LED0        18                  // led
#define PIN_GPIO_LED1        19                  // led

#define PIN_GPIO_LED_CON     PIN_GPIO_LED1       // shows connection state on the evaluation board
#define PIN_GPIO_SWITCH      PIN_GPIO_LED0       // toggle led 0

#define PIN_AIN_ADC          2                   // ain2 is p0.01 or gpio 1
#define PIN_AIN_LPCOMP       2                   // ain2 is p0.01 or gpio 1
#define PIN_AIN_LPCOMP_REF   0                   // ref0 is p0.00 or gpio 0
#define PIN_GPIO_RX          11                  // this is p0.11 or gpio 11
#define PIN_GPIO_TX          9                   // this is p0.09 or gpio 9
#define PIN_AIN_SENSOR       4

#define BUTTON_0             16
#define BUTTON_1             17

#endif

#if(BOARD==PCA10000)

#define LED_RGB_RED          21
#define LED_RGB_GREEN        22
#define LED_RGB_BLUE         23

#define PIN_GPIO_LED0        LED_RGB_RED         // led
#define PIN_GPIO_LED1        LED_RGB_GREEN       // led

#define PIN_GPIO_LED_CON     LED_RGB_GREEN       // shows connection state on the evaluation board
#define PIN_GPIO_SWITCH      LED_RGB_RED         // toggle red led

#define PIN_GPIO_RX          11                  // this is p0.11 or gpio 11
#define PIN_GPIO_TX          9                   // this is p0.09 or gpio 9

// undefined
#define PIN_AIN_ADC          2                   // ain2 is p0.01 or gpio 1
#define PIN_AIN_LPCOMP       2                   // ain2 is p0.01 or gpio 1
#define PIN_AIN_LPCOMP_REF   0                   // ref0 is p0.00 or gpio 0
//#define PIN_AIN_SENSOR       4

#endif

#if(BOARD==NRF51422)

#define PIN_GPIO_LED0        20                  // led
#define PIN_GPIO_LED1        21                  // led

#define PIN_GPIO_SWITCH     18
#define PIN_AIN_ADC          2                   // ain 2 is p0.01 or gpio 1
#define PIN_GPIO_RX          1
#define PIN_GPIO_TX          2

#endif

#if(BOARD==VIRTUALMEMO)

#define PIN_GPIO_LED0        7                   // this is p0.07 or gpio 7
#define PIN_GPIO_LED1        8                   // this is p0.08 or gpio 8
#define PIN_GPIO_LED2        9
#define PIN_GPIO_LED3        10
#define PIN_GPIO_LED4        11
#define PIN_GPIO_LED5        12
#define PIN_GPIO_LED6        13
#define PIN_GPIO_LED7        14

#define PIN_GPIO_RX          15
#define PIN_GPIO_TX          16

#define PIN_AIN_ADC          2                   // only here because it is required for the checks below, does not work
#define PIN_AIN_LPCOMP       3                   // only here because it is required for the checks below, does not work
#define PIN_AIN_LPCOMP_REF   0                   // only here because it is required for the checks below, does not work
#define PIN_GPIO_SWITCH      18                  // only here because it is required for the checks below, does not work

#endif

// Sanity checks

#ifndef PIN_AIN_ADC
#error "For AD conversion PIN_ADC must be defined"
#endif

#ifndef PIN_AIN_LPCOMP
#error "For LP comparison PIN_LPCOMP must be defined"
#endif

#ifndef PIN_GPIO_RX
#error "For UART, PIN_RX must be defined"
#endif

#ifndef PIN_GPIO_TX
#error "For UART, PIN_TX must be defined"
#endif

#ifndef HARDWARE_VERSION
#error "You have to specify the hardware version of your chip"
#endif

//#ifndef TICK_CONTINUOUSLY
//#error "You have to specify TICK_CONTINUOUSLY in your config file"
//#endif

