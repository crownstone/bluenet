/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#ifndef CS_BOARDS_T
#define CS_BOARDS_T

// Current accepted BOARD types

#define PCA10001             0
#define NRF6310              1
#define NRF51822_DONGLE      2
#define NRF51822_EVKIT       3
#define RFDUINO              4
#define CROWNSTONE           5
#define NRF51422             6

#ifndef BOARD
#error "Add BOARD=... to CMakeBuild.config"
#endif

#if(BOARD==RFDUINO)

#define PIN_RED              2                   // this is gpio 2 (bottom pin)
#define PIN_GREEN            3                   // this is gpio 3 (second pin)
#define PIN_BLUE             4                   // this is gpio 4 (third pin)

#endif


#if(BOARD==NRF6310)

#define PIN_LED              8                   // this is p1.0
#define PIN_ADC              2                   // ain2 is p0.1
#define PIN_RX               16
#define PIN_TX               17

#endif


#if(BOARD==CROWNSTONE)

#define PIN_LED              3                   // this is gpio 3
#define PIN_ADC              5                   // ain5 is pin 4

#endif


#if(BOARD==PCA10001)

#define PIN_LED              18
#define PIN_ADC              2                   // ain 2 is p0.1
#define PIN_RX               11
#define PIN_TX               9

#endif

#if(BOARD==NRF51422)

#define PIN_LED              18
#define PIN_ADC              2                   // ain 2 is p0.1
#define PIN_RX               1
#define PIN_TX               2

#endif

// Sanity check to see if all required pins are defined

#ifndef PIN_ADC
#error "For AD conversion PIN_ADC must be defined"
#endif

#ifndef PIN_RX
#error "For UART, PIN_RX must be defined"
#endif

#ifndef PIN_TX
#error "For UART, PIN_TX must be defined"
#endif


#endif // CS_BOARDS_T
