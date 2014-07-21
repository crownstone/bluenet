
#ifndef _pinconfig_h
#define _pinconfig_h

// Change this for different display size
// supported sizes: 1_44 2_0 2_7
#define EPD_SIZE EPD_2_7

#ifdef EPAPER_DEMO_BOARD

// This uses the EPaper demonstration board from repaper.org and Adafruit:
// http://repaper.org/doc/extension_board.html

// Pin Definitions:                 Nrf Port   P2 Pin      EPD Pin    EPD Color   Signal
                                 // -          1           1                      3.3V
const int Pin_TEMPERATURE = A6;  // AIN6       -           6          Green       Temp (analog)
const int Pin_Sclk        = 24;  // P0.24      7           7          Yellow      SCLK
const int Pin_BUSY        = 25;  // P0.25      8           8          Orange      Busy
const int Pin_PWM         = 20;  // P0.20 !!  33           9          Brown       PWM
const int Pin_RESET       = 21;  // P0.21 !!  34          10          Black       Reset
const int Pin_PANEL_ON    = 28;  // P0.28     11          11          Red         Panel On
const int Pin_DISCHARGE   = 29;  // P0.29     12          12          White       Discharge
const int Pin_BORDER      = 30;  // P0.30     13          13          Grey        Border
const int Pin_MISO        = 16;  // P0.16     29          14          Purple      MISO
const int Pin_MOSI        = 17;  // P0.17     30          15          Blue        MOSI
const int Pin_FLASH_CS    = 18;  // P0.18     31          18          Orange      Flash CS
const int Pin_EPD_CS      = 19;  // P0.19     32          19          Brown       EPD CS
                                 // -         39          20                      Gnd

const int Pin_SW2 = 10;          // none
const int Pin_LED = 13;

#undef HAVE_POWER_PINS

#elif defined(E_STICKY_v1) /* E_STICKY_v1, v1.1 */

// This is a custom board that combines the EPaper passives, the Nordic NRF51822
// and a power management chip (LTC 3554), among other components.

// Pin Definitions:                 Nrf Port   Nrf Pin
static const int Pin_TEMPERATURE = A6;  // AIN6       -
static const int Pin_Sclk        = 10;  // P0.10     16
static const int Pin_BUSY        =  9;  // P0.9      15
static const int Pin_PWM         = 16;  // P0.16     22
static const int Pin_RESET       = 13;  // P0.13     19
static const int Pin_PANEL_ON    = 18;  // P0.18     26  // LTC 3554 PWR_ON1
static const int Pin_DISCHARGE   = 15;  // P0.15     21
static const int Pin_BORDER      = 14;  // P0.14     20
static const int Pin_MISO        = 12;  // P0.12     18
static const int Pin_MOSI        = 11;  // P0.11     17
static const int Pin_FLASH_CS    = 20;  // P0.20     31 // unused
static const int Pin_EPD_CS      =  8;  // P0.8      14

static const int Pin_SW2 = 7;           // P0.07     11
static const int Pin_LED = 30;          // P0.30      3  (blue)

static const int Pin_uSD_CS = 24;   // P0.24 (43): Micro SD CS
static const int Pin_uSD_MOSI = 25; // P0.25 (44): Micro SD MOSI
static const int Pin_uSD_CLK = 28;  // P0.28 (47): Micro SD CLK
static const int Pin_uSD_MISO = 29; // P0.29 (48): Micro SD MISO

#define HAVE_POWER_PINS

const int Pin_Standby     = 17;  // P.17      25  // LTC 3554 Standby
const int Pin_PowerOn_2   = 19;  // P.18      27  // LTC 4553 PWR_ON2

#else

#error "No board defined."

#endif  /* EPAPER_DEMO_BOARD */


void setup_power() {
    #ifdef HAVE_POWER_PINS


    NRF51_GPIO_PIN_CNF(Pin_Standby) = NRF51_GPIO_PIN_CNF_OUTPUT;
    NRF51_GPIO_OUTCLR = 1 << Pin_Standby;
    NRF51_GPIO_PIN_CNF(Pin_PowerOn_2) = NRF51_GPIO_PIN_CNF_OUTPUT;
    NRF51_GPIO_OUTSET = 1 << Pin_PowerOn_2;
    #endif /* HAVE_POWER_PINS */

    NRF51_POWER_LOWPWR = 1;

}

#endif /* _pinconfig_h */