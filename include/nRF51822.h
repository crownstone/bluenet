#ifndef __nRF51822_h
#define __nRF51822_h

#ifndef NULL
#define NULL ((void *)0)
#endif

#include <stdint.h>

// This in some sense replicates the Nordic SDK headers.  I wanted something that
// wasn't covered by their license, and that used plain offsets instead of structs.

// To format register tables from reference manual,
// for instance: PORT 0x17C Event generate from multiple input pins.
// Replace ^([^/ #\n\r]+) (\S+) (.*)
// With    #define FOO_\1                *(volatile uint32_t*) (FOO + \2)          //\3
// Where FOO is name of peripheral

// For registers with brackets,
// for instance: CONFIG[0] 0x510 Configuration for OUT[0] task and IN[0] event.
// Replace ^([^/ #\n\r]+)\[(\d+)\] (\S+) (.*)
// With    #define NRF51_GPIOTE_$1_$2                *(volatile uint32_t*) (NRF51_GPIOTE + $3)          // $4


////////////////////////////////////////////////////////////////////////////////////////////////////
/// NRF51_POWER //////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//TASKS
#define NRF51_POWER                    (volatile uint8_t*) 0x4000B000
#define NRF51_POWER_CONSTLAT           *(volatile uint32_t*) (NRF51_POWER + 0x078)          //Enable constant latency mode
#define NRF51_POWER_LOWPWR             *(volatile uint32_t*) (NRF51_POWER + 0x07C)          //Enable low power mode (variable latency)
//EVENTS
#define NRF51_POWER_POFWARN            *(volatile uint32_t*) (NRF51_POWER + 0x108)          //Power failure warning
//REGISTERS
#define NRF51_POWER_INTENSET           *(volatile uint32_t*) (NRF51_POWER + 0x304)          //Interrupt enable set register
#define NRF51_POWER_INTENCLR           *(volatile uint32_t*) (NRF51_POWER + 0x308)          //Interrupt enable clear register
#define NRF51_POWER_RESETREAS          *(volatile uint32_t*) (NRF51_POWER + 0x400)          //Reset reason
#define NRF51_POWER_SYSTEMOFF          *(volatile uint32_t*) (NRF51_POWER + 0x500)          //System off register
#define NRF51_POWER_POFCON             *(volatile uint32_t*) (NRF51_POWER + 0x510)          //Power failure configuration
#define NRF51_POWER_GPREGRET           *(volatile uint32_t*) (NRF51_POWER + 0x51C)          //General purpose retention register
#define NRF51_POWER_RAMON              *(volatile uint32_t*) (NRF51_POWER + 0x524)          //RAM on/off
#define NRF51_POWER_RESET              *(volatile uint32_t*) (NRF51_POWER + 0x544)          //Configure reset functionality
#define NRF51_POWER_DCDCEN             *(volatile uint32_t*) (NRF51_POWER + 0x578)          //DCDC enable register

////////////////////////////////////////////////////////////////////////////////////////////////////
/// CLOCK //////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//TASKS
#define NRF51_CLOCK                     (volatile uint8_t*) 0x40000000
#define NRF51_CLOCK_HFCLKSTART          *(volatile uint32_t*) (NRF51_CLOCK + 0x000)          //Start HFCLK crystal oscillator
#define NRF51_CLOCK_HFCLKSTOP           *(volatile uint32_t*) (NRF51_CLOCK + 0x004)          //Stop HFCLK crystal oscillator
#define NRF51_CLOCK_LFCLKSTART          *(volatile uint32_t*) (NRF51_CLOCK + 0x008)          //Start LFCLK source
#define NRF51_CLOCK_LFCLKSTOP           *(volatile uint32_t*) (NRF51_CLOCK + 0x00C)          //Stop LFCLK source
#define NRF51_CLOCK_CAL                 *(volatile uint32_t*) (NRF51_CLOCK + 0x010)          //Start calibration of LFCLK RC oscillator
#define NRF51_CLOCK_CTSTART             *(volatile uint32_t*) (NRF51_CLOCK + 0x014)          //Start calibration timer
#define NRF51_CLOCK_CTSTOP              *(volatile uint32_t*) (NRF51_CLOCK + 0x018)          //Stop calibration timer
//EVENTS
#define NRF51_CLOCK_HFCLKSTARTED        *(volatile uint32_t*) (NRF51_CLOCK + 0x100)          //16 MHz oscillator started
#define NRF51_CLOCK_LFCLKSTARTED        *(volatile uint32_t*) (NRF51_CLOCK + 0x104)          //32 kHz oscillator started
#define NRF51_CLOCK_DONE                *(volatile uint32_t*) (NRF51_CLOCK + 0x10C)          //Calibration of LFCLK RC oscillator complete event
#define NRF51_CLOCK_CTTO                *(volatile uint32_t*) (NRF51_CLOCK + 0x110)          //Calibration timer timeout
//REGISTERS
#define NRF51_CLOCK_INTENSET            *(volatile uint32_t*) (NRF51_CLOCK + 0x304)          //Interrupt enable set register
#define NRF51_CLOCK_INTENCLR            *(volatile uint32_t*) (NRF51_CLOCK + 0x308)          //Interrupt enable clear register
#define NRF51_CLOCK_HFCLKSTAT           *(volatile uint32_t*) (NRF51_CLOCK + 0x40C)          //Which HFCLK source is running
#define NRF51_CLOCK_LFCLKSTAT           *(volatile uint32_t*) (NRF51_CLOCK + 0x418)          //Which LFCLK source is running
#define NRF51_CLOCK_LFCLKSRC            *(volatile uint32_t*) (NRF51_CLOCK + 0x518)          //Clock source for the 32 kHz clock
#define NRF51_CLOCK_LFCLKSRC_RC         0                                                    //32.768 kHz RC oscillator
#define NRF51_CLOCK_LFCLKSRC_XTAL       1                                                    //32.768 kHz crystal oscillator
#define NRF51_CLOCK_LFCLKSRC_SYNTH      2                                                    //32.768 kHz synthesizer synthesizing 32.768 kHz from 16 MHz system clock
#define NRF51_CLOCK_CTIV                *(volatile uint32_t*) (NRF51_CLOCK + 0x538)          //Calibration timer interval
#define NRF51_CLOCK_XTALFREQ            *(volatile uint32_t*) (NRF51_CLOCK + 0x550)          //Crystal frequency - DOES NOT WORK - see PAN 028 "12. HFCLK: XTALFREQ register is not functional"


////////////////////////////////////////////////////////////////////////////////////////////////////
/// TIMERS //////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// Note: TIMER0 is blocked by the softdevice when the latter is enabled.


// TASKS
#define NRF51_TIMER0                     (volatile uint8_t*) 0x40008000
#define NRF51_TIMER0_IRQn                8
#define NRF51_TIMER0_START               *(volatile uint32_t*) (NRF51_TIMER0 + 0x000)       //Start Timer
#define NRF51_TIMER0_STOP                *(volatile uint32_t*) (NRF51_TIMER0 + 0x004)       //Stop Timer
#define NRF51_TIMER0_COUNT               *(volatile uint32_t*) (NRF51_TIMER0 + 0x008)       //Increment Timer (Counter mode only)
#define NRF51_TIMER0_CLEAR               *(volatile uint32_t*) (NRF51_TIMER0 + 0x00C)       //Clear timer
#define NRF51_TIMER0_CAPTURE_0           *(volatile uint32_t*) (NRF51_TIMER0 + 0x040)       //Capture Timer value to CC0 register
#define NRF51_TIMER0_CAPTURE_1           *(volatile uint32_t*) (NRF51_TIMER0 + 0x044)       //Capture Timer value to CC1 register
#define NRF51_TIMER0_CAPTURE_2           *(volatile uint32_t*) (NRF51_TIMER0 + 0x048)       //Capture Timer value to CC2 register
#define NRF51_TIMER0_CAPTURE_3           *(volatile uint32_t*) (NRF51_TIMER0 + 0x04C)       //Capture Timer value to CC3 register
//EVENTS
#define NRF51_TIMER0_COMPARE_0           *(volatile uint32_t*) (NRF51_TIMER0 + 0x140)       //Compare event on CC[0] match
#define NRF51_TIMER0_COMPARE_1           *(volatile uint32_t*) (NRF51_TIMER0 + 0x144)       //Compare event on CC[1] match
#define NRF51_TIMER0_COMPARE_2           *(volatile uint32_t*) (NRF51_TIMER0 + 0x148)       //Compare event on CC[2] match
#define NRF51_TIMER0_COMPARE_3           *(volatile uint32_t*) (NRF51_TIMER0 + 0x14C)       //Compare event on CC[3] match
//REGISTERS
#define NRF51_TIMER0_SHORTS              *(volatile uint32_t*) (NRF51_TIMER0 + 0x200)       //Shortcuts
#define NRF51_TIMER0_INTENSET            *(volatile uint32_t*) (NRF51_TIMER0 + 0x304)       //Write-only - configures which events generate a Timer interrupt
#define NRF51_TIMER0_INTENCLR            *(volatile uint32_t*) (NRF51_TIMER0 + 0x308)       //Write-only - configures which events do not generate a Timer interrupt
#define NRF51_TIMER0_MODE                *(volatile uint32_t*) (NRF51_TIMER0 + 0x504)       //Timer mode selection
#define NRF51_TIMER0_MODE_TIMER 0
#define NRF51_TIMER0_MODE_COUNTER 1
#define NRF51_TIMER0_BITMODE_16_BIT 0
#define NRF51_TIMER0_BITMODE_8_BIT 1
#define NRF51_TIMER0_BITMODE_24_BIT 2
#define NRF51_TIMER0_BITMODE_32_BIT 3
#define NRF51_TIMER0_BITMODE             *(volatile uint32_t*) (NRF51_TIMER0 + 0x508)       //Configure the number of bits used by the TIMER
#define NRF51_TIMER0_PRESCALER           *(volatile uint32_t*) (NRF51_TIMER0 + 0x510)       //Timer prescaler register
#define NRF51_TIMER0_CC_0                *(volatile uint32_t*) (NRF51_TIMER0 + 0x540)       //Capture/Compare register 0
#define NRF51_TIMER0_CC_1                *(volatile uint32_t*) (NRF51_TIMER0 + 0x544)       //Capture/Compare register 1
#define NRF51_TIMER0_CC_2                *(volatile uint32_t*) (NRF51_TIMER0 + 0x548)       //Capture/Compare register 2
#define NRF51_TIMER0_CC_3                *(volatile uint32_t*) (NRF51_TIMER0 + 0x54C)       //Capture/Compare register 3


// TASKS
#define NRF51_TIMER1                     (volatile uint8_t*) 0x40009000
#define NRF51_TIMER1_IRQn                9
#define NRF51_TIMER1_START               *(volatile uint32_t*) (NRF51_TIMER1 + 0x000)       //Start Timer
#define NRF51_TIMER1_STOP                *(volatile uint32_t*) (NRF51_TIMER1 + 0x004)       //Stop Timer
#define NRF51_TIMER1_COUNT               *(volatile uint32_t*) (NRF51_TIMER1 + 0x008)       //Increment Timer (Counter mode only)
#define NRF51_TIMER1_CLEAR               *(volatile uint32_t*) (NRF51_TIMER1 + 0x00C)       //Clear timer
#define NRF51_TIMER1_CAPTURE_0           *(volatile uint32_t*) (NRF51_TIMER1 + 0x040)       //Capture Timer value to CC0 register
#define NRF51_TIMER1_CAPTURE_1           *(volatile uint32_t*) (NRF51_TIMER1 + 0x044)       //Capture Timer value to CC1 register
#define NRF51_TIMER1_CAPTURE_2           *(volatile uint32_t*) (NRF51_TIMER1 + 0x048)       //Capture Timer value to CC2 register
#define NRF51_TIMER1_CAPTURE_3           *(volatile uint32_t*) (NRF51_TIMER1 + 0x04C)       //Capture Timer value to CC3 register
//EVENTS
#define NRF51_TIMER1_COMPARE_0           *(volatile uint32_t*) (NRF51_TIMER1 + 0x140)       //Compare event on CC[0] match
#define NRF51_TIMER1_COMPARE_1           *(volatile uint32_t*) (NRF51_TIMER1 + 0x144)       //Compare event on CC[1] match
#define NRF51_TIMER1_COMPARE_2           *(volatile uint32_t*) (NRF51_TIMER1 + 0x148)       //Compare event on CC[2] match
#define NRF51_TIMER1_COMPARE_3           *(volatile uint32_t*) (NRF51_TIMER1 + 0x14C)       //Compare event on CC[3] match
#define NRF51_EVENT_INTERRUPT_N(perif, event) (1<<(((((uint32_t)&event) - ((uint32_t)perif)) - 0x100) >> 2))
//REGISTERS
#define NRF51_TIMER1_SHORTS              *(volatile uint32_t*) (NRF51_TIMER1 + 0x200)       //Shortcuts
#define NRF51_TIMER_SHORTS_COMPARE0_CLEAR (1 << 0)
#define NRF51_TIMER_SHORTS_COMPARE1_CLEAR (1 << 1)
#define NRF51_TIMER_SHORTS_COMPARE2_CLEAR (1 << 2)
#define NRF51_TIMER_SHORTS_COMPARE3_CLEAR (1 << 3)
#define NRF51_TIMER_SHORTS_COMPARE0_STOP  (1 << 8)
#define NRF51_TIMER_SHORTS_COMPARE1_STOP  (1 << 9)
#define NRF51_TIMER_SHORTS_COMPARE2_STOP  (1 << 10)
#define NRF51_TIMER_SHORTS_COMPARE3_STOP  (1 << 11)


#define NRF51_TIMER1_INTENSET            *(volatile uint32_t*) (NRF51_TIMER1 + 0x304)       //Write-only - configures which events generate a Timer interrupt
#define NRF51_TIMER1_INTENCLR            *(volatile uint32_t*) (NRF51_TIMER1 + 0x308)       //Write-only - configures which events do not generate a Timer interrupt
#define NRF51_TIMER1_MODE                *(volatile uint32_t*) (NRF51_TIMER1 + 0x504)       //Timer mode selection
#define NRF51_TIMER1_MODE_TIMER 0
#define NRF51_TIMER1_MODE_COUNTER 1
#define NRF51_TIMER1_BITMODE_16_BIT 0
#define NRF51_TIMER1_BITMODE_8_BIT 1
#define NRF51_TIMER1_BITMODE_24_BIT 2
#define NRF51_TIMER1_BITMODE_32_BIT 3
#define NRF51_TIMER1_BITMODE             *(volatile uint16_t*) (NRF51_TIMER1 + 0x508)       //Configure the number of bits used by the TIMER
#define NRF51_TIMER1_PRESCALER           *(volatile uint32_t*) (NRF51_TIMER1 + 0x510)       //Timer prescaler register
#define NRF51_TIMER1_CC_0                *(volatile uint32_t*) (NRF51_TIMER1 + 0x540)       //Capture/Compare register 0
#define NRF51_TIMER1_CC_1                *(volatile uint32_t*) (NRF51_TIMER1 + 0x544)       //Capture/Compare register 1
#define NRF51_TIMER1_CC_2                *(volatile uint32_t*) (NRF51_TIMER1 + 0x548)       //Capture/Compare register 2
#define NRF51_TIMER1_CC_3                *(volatile uint32_t*) (NRF51_TIMER1 + 0x54C)       //Capture/Compare register 3


// TASKS
#define NRF51_TIMER2                     (volatile uint8_t*) 0x4000A000
#define NRF51_TIMER2_IRQn                10
#define NRF51_TIMER2_START               *(volatile uint32_t*) (NRF51_TIMER2 + 0x000)       //Start Timer
#define NRF51_TIMER2_STOP                *(volatile uint32_t*) (NRF51_TIMER2 + 0x004)       //Stop Timer
#define NRF51_TIMER2_COUNT               *(volatile uint32_t*) (NRF51_TIMER2 + 0x008)       //Increment Timer (Counter mode only)
#define NRF51_TIMER2_CLEAR               *(volatile uint32_t*) (NRF51_TIMER2 + 0x00C)       //Clear timer
#define NRF51_TIMER2_CAPTURE_0           *(volatile uint32_t*) (NRF51_TIMER2 + 0x040)       //Capture Timer value to CC0 register
#define NRF51_TIMER2_CAPTURE_1           *(volatile uint32_t*) (NRF51_TIMER2 + 0x044)       //Capture Timer value to CC1 register
#define NRF51_TIMER2_CAPTURE_2           *(volatile uint32_t*) (NRF51_TIMER2 + 0x048)       //Capture Timer value to CC2 register
#define NRF51_TIMER2_CAPTURE_3           *(volatile uint32_t*) (NRF51_TIMER2 + 0x04C)       //Capture Timer value to CC3 register
//EVENTS
#define NRF51_TIMER2_COMPARE_0           *(volatile uint32_t*) (NRF51_TIMER2 + 0x140)       //Compare event on CC[0] match
#define NRF51_TIMER2_COMPARE_1           *(volatile uint32_t*) (NRF51_TIMER2 + 0x144)       //Compare event on CC[1] match
#define NRF51_TIMER2_COMPARE_2           *(volatile uint32_t*) (NRF51_TIMER2 + 0x148)       //Compare event on CC[2] match
#define NRF51_TIMER2_COMPARE_3           *(volatile uint32_t*) (NRF51_TIMER2 + 0x14C)       //Compare event on CC[3] match
//REGISTERS
#define NRF51_TIMER2_SHORTS              *(volatile uint32_t*) (NRF51_TIMER2 + 0x200)       //Shortcuts
#define NRF51_TIMER2_INTENSET            *(volatile uint32_t*) (NRF51_TIMER2 + 0x304)       //Write-only - configures which events generate a Timer interrupt
#define NRF51_TIMER2_INTENCLR            *(volatile uint32_t*) (NRF51_TIMER2 + 0x308)       //Write-only - configures which events do not generate a Timer interrupt
#define NRF51_TIMER2_MODE                *(volatile uint32_t*) (NRF51_TIMER2 + 0x504)       //Timer mode selection
#define NRF51_TIMER2_MODE_TIMER 0
#define NRF51_TIMER2_MODE_COUNTER 1
#define NRF51_TIMER2_BITMODE_16_BIT 0
#define NRF51_TIMER2_BITMODE_8_BIT 1
#define NRF51_TIMER2_BITMODE_24_BIT 2
#define NRF51_TIMER2_BITMODE_32_BIT 3
#define NRF51_TIMER2_BITMODE             *(volatile uint32_t*) (NRF51_TIMER2 + 0x508)       //Configure the number of bits used by the TIMER
#define NRF51_TIMER2_PRESCALER           *(volatile uint32_t*) (NRF51_TIMER2 + 0x510)       //Timer prescaler register
#define NRF51_TIMER2_CC_0                *(volatile uint32_t*) (NRF51_TIMER2 + 0x540)       //Capture/Compare register 0
#define NRF51_TIMER2_CC_1                *(volatile uint32_t*) (NRF51_TIMER2 + 0x544)       //Capture/Compare register 1
#define NRF51_TIMER2_CC_2                *(volatile uint32_t*) (NRF51_TIMER2 + 0x548)       //Capture/Compare register 2
#define NRF51_TIMER2_CC_3                *(volatile uint32_t*) (NRF51_TIMER2 + 0x54C)       //Capture/Compare register 3


////////////////////////////////////////////////////////////////////////////////////////////////////
/// RTC ////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


// Note RTC0 is used by softdevice when the latter is enabled.
// TASKS
#define NRF51_RTC0                      (volatile uint8_t*) 0x4000B000
#define NRF51_RTC0_IRQn                 11
#define NRF51_RTC0_START                *(volatile uint32_t*) (NRF51_RTC0 + 0x000)          //Start RTC COUNTER
#define NRF51_RTC0_STOP                 *(volatile uint32_t*) (NRF51_RTC0 + 0x004)          //Stop RTC COUNTER
#define NRF51_RTC0_CLEAR                *(volatile uint32_t*) (NRF51_RTC0 + 0x008)          //Clear RTC COUNTER
#define NRF51_RTC0_TRIGOVRFLW           *(volatile uint32_t*) (NRF51_RTC0 + 0x00C)          //Set COUNTER to 0xFFFFF0
// EVENTS
#define NRF51_RTC0_TICK                 *(volatile uint32_t*) (NRF51_RTC0 + 0x100)          //Event on COUNTER increment
#define NRF51_RTC0_OVRFLW               *(volatile uint32_t*) (NRF51_RTC0 + 0x104)          //Event on COUNTER overflow
#define NRF51_RTC0_COMPARE_0            *(volatile uint32_t*) (NRF51_RTC0 + 0x140)          //Compare event on CC[0] match
#define NRF51_RTC0_COMPARE_1            *(volatile uint32_t*) (NRF51_RTC0 + 0x144)          //Compare event on CC[1] match
#define NRF51_RTC0_COMPARE_2            *(volatile uint32_t*) (NRF51_RTC0 + 0x148)          //Compare event on CC[2] match
#define NRF51_RTC0_COMPARE_3            *(volatile uint32_t*) (NRF51_RTC0 + 0x14C)          //Compare event on CC[3] match
// REGISTERS
#define NRF51_RTC0_INTENSET             *(volatile uint32_t*) (NRF51_RTC0 + 0x304)          //Configures which events shall generate a RTC interrupt
#define NRF51_RTC0_INTENCLR             *(volatile uint32_t*) (NRF51_RTC0 + 0x308)          //Configures which events shall not generate a RTC interrupt
#define NRF51_RTC0_EVTEN                *(volatile uint32_t*) (NRF51_RTC0 + 0x340)          //Configures event enable state for each RTC event
#define NRF51_RTC0_EVTEN_TICK           1
#define NRF51_RTC0_EVTEN_OVERFLOW       2
#define NRF51_RTC0_EVTEN_COMPAREN(n)    (16+n)
#define NRF51_RTC0_EVTEN_COMPARE0       NRF51_RTC0_EVTEN_COMPAREN(0)
#define NRF51_RTC0_EVTEN_COMPARE1       NRF51_RTC0_EVTEN_COMPAREN(1)
#define NRF51_RTC0_EVTEN_COMPARE2       NRF51_RTC0_EVTEN_COMPAREN(2)
#define NRF51_RTC0_EVTEN_COMPARE3       NRF51_RTC0_EVTEN_COMPAREN(3)
#define NRF51_RTC0_EVTENSET             *(volatile uint32_t*) (NRF51_RTC0 + 0x344)          //Enable event(s). Read of this register gives the value of EVTEN.
#define NRF51_RTC0_EVTENCLR             *(volatile uint32_t*) (NRF51_RTC0 + 0x348)          //Disable event(s). Read of this register gives the value of EVTEN.
#define NRF51_RTC0_COUNTER              *(volatile uint32_t*) (NRF51_RTC0 + 0x504)          //Current COUNTER value
#define NRF51_RTC0_PRESCALER            *(volatile uint32_t*) (NRF51_RTC0 + 0x508)          //12-bit prescaler for COUNTER frequency (32768/(PRESCALER+1)) Must be written when RTC is stopped
#define NRF51_RTC0_CC_0                 *(volatile uint32_t*) (NRF51_RTC0 + 0x540)          //Compare register
#define NRF51_RTC0_CC_1                 *(volatile uint32_t*) (NRF51_RTC0 + 0x544)          //Compare register

// TASKS
#define NRF51_RTC1                      (volatile uint8_t*) 0x40011000
#define NRF51_RTC1_IRQn                 17
#define NRF51_RTC1_START                *(volatile uint32_t*) (NRF51_RTC1 + 0x000)          //Start RTC COUNTER
#define NRF51_RTC1_STOP                 *(volatile uint32_t*) (NRF51_RTC1 + 0x004)          //Stop RTC COUNTER
#define NRF51_RTC1_CLEAR                *(volatile uint32_t*) (NRF51_RTC1 + 0x008)          //Clear RTC COUNTER
#define NRF51_RTC1_TRIGOVRFLW           *(volatile uint32_t*) (NRF51_RTC1 + 0x00C)          //Set COUNTER to 0xFFFFF0
// EVENTS
#define NRF51_RTC1_TICK                 *(volatile uint32_t*) (NRF51_RTC1 + 0x100)          //Event on COUNTER increment
#define NRF51_RTC1_OVRFLW               *(volatile uint32_t*) (NRF51_RTC1 + 0x104)          //Event on COUNTER overflow
#define NRF51_RTC1_COMPARE_0            *(volatile uint32_t*) (NRF51_RTC1 + 0x140)          //Compare event on CC[0] match
#define NRF51_RTC1_COMPARE_1            *(volatile uint32_t*) (NRF51_RTC1 + 0x144)          //Compare event on CC[1] match
#define NRF51_RTC1_COMPARE_2            *(volatile uint32_t*) (NRF51_RTC1 + 0x148)          //Compare event on CC[2] match
#define NRF51_RTC1_COMPARE_3            *(volatile uint32_t*) (NRF51_RTC1 + 0x14C)          //Compare event on CC[3] match
// REGISTERS
#define NRF51_RTC1_INTENSET             *(volatile uint32_t*) (NRF51_RTC1 + 0x304)          //Configures which events shall generate a RTC interrupt
#define NRF51_RTC1_INTENCLR             *(volatile uint32_t*) (NRF51_RTC1 + 0x308)          //Configures which events shall not generate a RTC interrupt
#define NRF51_RTC1_EVTEN                *(volatile uint32_t*) (NRF51_RTC1 + 0x340)          //Configures event enable state for each RTC event
#define NRF51_RTC1_EVTEN_TICK           1
#define NRF51_RTC1_EVTEN_OVERFLOW       2
#define NRF51_RTC1_EVTEN_COMPAREN(n)    (1<<(16+n))
#define NRF51_RTC1_EVTEN_COMPARE0       NRF51_RTC1_EVTEN_COMPAREN(0)
#define NRF51_RTC1_EVTEN_COMPARE1       NRF51_RTC1_EVTEN_COMPAREN(1)
#define NRF51_RTC1_EVTEN_COMPARE2       NRF51_RTC1_EVTEN_COMPAREN(2)
#define NRF51_RTC1_EVTEN_COMPARE3       NRF51_RTC1_EVTEN_COMPAREN(3)
#define NRF51_RTC1_EVTENSET             *(volatile uint32_t*) (NRF51_RTC1 + 0x344)          //Enable event(s). Read of this register gives the value of EVTEN.
#define NRF51_RTC1_EVTENCLR             *(volatile uint32_t*) (NRF51_RTC1 + 0x348)          //Disable event(s). Read of this register gives the value of EVTEN.
#define NRF51_RTC1_COUNTER              *(volatile uint32_t*) (NRF51_RTC1 + 0x504)          //Current COUNTER value
#define NRF51_RTC1_COUNTER_MAX          (1<<24)                                             //Max COUNTER value
#define NRF51_RTC1_PRESCALER            *(volatile uint32_t*) (NRF51_RTC1 + 0x508)          //12-bit prescaler for COUNTER frequency (32768/(PRESCALER+1)) Must be written when RTC is stopped
#define NRF51_RTC1_CC_0                 *(volatile uint32_t*) (NRF51_RTC1 + 0x540)          //Compare register
#define NRF51_RTC1_CC_1                 *(volatile uint32_t*) (NRF51_RTC1 + 0x544)          //Compare register


////////////////////////////////////////////////////////////////////////////////////////////////////
/// General Purpose Input Output (GPIO) ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define NRF51_GPIO                      (volatile uint8_t*) 0x50000000
#define NRF51_GPIO_OUT                  *(volatile uint32_t*) (NRF51_GPIO + 0x504)
#define NRF51_GPIO_OUTSET               *(volatile uint32_t*) (NRF51_GPIO + 0x508)
#define NRF51_GPIO_OUTCLR               *(volatile uint32_t*) (NRF51_GPIO + 0x50C)
#define NRF51_GPIO_IN                   *(volatile uint32_t*) (NRF51_GPIO + 0x510)
#define NRF51_GPIO_DIR                  *(volatile uint32_t*) (NRF51_GPIO + 0x514)
#define NRF51_GPIO_DIRSET               *(volatile uint32_t*) (NRF51_GPIO + 0x518)          // setting a bit to 1 sets the corresponding pin to output.
#define NRF51_GPIO_DIRCLR               *(volatile uint32_t*) (NRF51_GPIO + 0x518)          // setting a bit to 1 sets the corresponding pin to input.
#define NRF51_GPIO_DIR_OUTPUT(n)        NRF51_GPIO_DIRSET = 1 << n
#define NRF51_GPIO_DIR_INPUT (n)        NRF51_GPIO_DIRCLR = 1 << n
#define NRF51_GPIO_PIN_CNF(n)           *(volatile uint32_t*) (NRF51_GPIO + 0x700 + (n*4))
#define NRF51_GPIO_PIN_CNF_INPUT            ~1
#define NRF51_GPIO_PIN_CNF_OUTPUT            1
#define NRF51_GPIO_PIN_CNF_INPUT_CONNECT    ~0b00
#define NRF51_GPIO_PIN_CNF_INPUT_DISCONNECT  0b10
#define NRF51_GPIO_PIN_CNF_PULL_DISABLED    ~0b1100
#define NRF51_GPIO_PIN_CNF_PULL_PULLDOWN    0b0100
#define NRF51_GPIO_PIN_CNF_PULL_PULLUP      0b1000
#define NRF51_GPIO_PIN_CNF_DRIVE_S0S1       0b100000000
#define NRF51_GPIO_PIN_CNF_SENSE_HIGH        1<<16                                            // Pin sensing mechanism: Sense for high level
#define NRF51_GPIO_PIN_CNF_SENSE_LOW         1<<17                                            // Pin sensing mechanism: Sense for low level



////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPIO TASKS AND EVENTS //////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// TASKS
#define NRF51_GPIOTE                    (volatile uint8_t*) 0x40006000
#define NRF51_GPIOTE_IRQn               6
#define NRF51_GPIOTE_OUT_N(n)           *(volatile uint32_t*) (NRF51_GPIOTE + 0x000 + (n*4))  // Task for writing to pin specified by PSEL in CONFIG[n].
#define NRF51_GPIOTE_OUT_0              *(volatile uint32_t*) (NRF51_GPIOTE + 0x000)          // Task for writing to pin specified by PSEL in CONFIG[0].
#define NRF51_GPIOTE_OUT_1              *(volatile uint32_t*) (NRF51_GPIOTE + 0x004)          // Task for writing to pin specified by PSEL in CONFIG[1].
#define NRF51_GPIOTE_OUT_2              *(volatile uint32_t*) (NRF51_GPIOTE + 0x008)          // Task for writing to pin specified by PSEL in CONFIG[2].
#define NRF51_GPIOTE_OUT_3              *(volatile uint32_t*) (NRF51_GPIOTE + 0x00C)          // Task for writing to pin specified by PSEL in CONFIG[3].
// EVENTS
#define NRF51_GPIOTE_IN_N(n)            *(volatile uint32_t*) (NRF51_GPIOTE + 0x100 + (n*4))  // Event generated from pin specified by PSEL in CONFIG[n].
#define NRF51_GPIOTE_IN_0               *(volatile uint32_t*) (NRF51_GPIOTE + 0x100)          // Event generated from pin specified by PSEL in CONFIG[0].
#define NRF51_GPIOTE_IN_1               *(volatile uint32_t*) (NRF51_GPIOTE + 0x104)          // Event generated from pin specified by PSEL in CONFIG[1].
#define NRF51_GPIOTE_IN_2               *(volatile uint32_t*) (NRF51_GPIOTE + 0x108)          // Event generated from pin specified by PSEL in CONFIG[2].
#define NRF51_GPIOTE_IN_3               *(volatile uint32_t*) (NRF51_GPIOTE + 0x10C)          // Event generated from pin specified by PSEL in CONFIG[3].
#define NRF51_GPIOTE_PORT               *(volatile uint32_t*) (NRF51_GPIOTE + 0x17C)          // Event generate from multiple input pins.
// REGISTERS
#define NRF51_GPIOTE_INTENSET           *(volatile uint32_t*) (NRF51_GPIOTE + 0x304)          // Interrupt enable set register.
#define NRF51_GPIOTE_INTENCLR           *(volatile uint32_t*) (NRF51_GPIOTE + 0x308)          // Interrupt enable clear register.
#define NRF51_GPIOTE_CONFIG_N(n)        *(volatile uint32_t*) (NRF51_GPIOTE + 0x510 + (n*4))  // Configuration for OUT[n] task and IN[n] event.
#define NRF51_GPIOTE_CONFIG_0           *(volatile uint32_t*) (NRF51_GPIOTE + 0x510)          // Configuration for OUT[0] task and IN[0] event.
#define NRF51_GPIOTE_CONFIG_1           *(volatile uint32_t*) (NRF51_GPIOTE + 0x514)          // Configuration for OUT[1] task and IN[1] event.
#define NRF51_GPIOTE_CONFIG_2           *(volatile uint32_t*) (NRF51_GPIOTE + 0x518)          // Configuration for OUT[2] task and IN[2] event.
#define NRF51_GPIOTE_CONFIG_3           *(volatile uint32_t*) (NRF51_GPIOTE + 0x51C)          // Configuration for OUT[3] task and IN[3] event.


#define NRF51_GPIOTE_CONFIG_MODE_DISABLED     0                                               // Disabled. Pin specified by PSEL will not be acquired by the GPIOTE module.
#define NRF51_GPIOTE_CONFIG_MODE_EVENT        1                                               // Event mode. The pin specified by PSEL will be configured as an input and the IN[n] event will be generated if operation specified in POLARITY occurs on the pin.
#define NRF51_GPIOTE_CONFIG_MODE_TASK         3                                               // Task mode. The pin specified by PSEL will be configured as an output and triggering the OUT[n] task will perform the operation specified by POLARITY on the pin. When enabled as a task the GPIOTE module will acquire the pin and the pin can no longer be written as a regular output pin from the GPIO module.
#define NRF51_GPIOTE_CONFIG_PSEL(pin)         (pin << 8)                                      // Pin number associated with OUT[n] task and IN[n] event.
#define NRF51_GPIOTE_CONFIG_PSEL_NONE         (31 << 8)                                       // Disabled (pin 31)
#define NRF51_GPIOTE_CONFIG_POLARITY_LOTOHI   (1 << 16)                                       // Task mode: Set pin from OUT[n] task.  Event mode: Generate IN[n] event when rising edge on pin.
#define NRF51_GPIOTE_CONFIG_POLARITY_HITOLO   (2 << 16)                                       // Task mode: Clear pin from OUT[n] task.  Event mode: Generate IN[n] event when falling edge on pin.
#define NRF51_GPIOTE_CONFIG_POLARITY_TOGGLE   (3 << 16)                                       // Task mode: Toggle pin from OUT[n].  Event mode: Generate IN[n] when any change on pin.
#define NRF51_GPIOTE_CONFIG_OUTINIT_LOW       (~(1 << 20))                                    // Task mode: Initial value of pin before task triggering is low.
#define NRF51_GPIOTE_CONFIG_OUTINIT_HIGH      (1 << 20)                                       // Task mode: Initial value of pin before task triggering is high.



////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPIO TASKS AND EVENTS //////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


// Note: when softdevice is enabled, only PPI channels <= 7 are available to the application
// Also: when softdevice is enabled, use the softdevice functions

#define NRF51_PPI                       (volatile uint8_t*)0x4001F000
//TASKS
#define NRF51_PPI_CHG_EN_N(n)           *(volatile uint32_t*) (NRF51_PPI + 0x000 + (n*8))     // Enable channel group N
#define NRF51_PPI_CHG_DIS_N(n)          *(volatile uint32_t*) (NRF51_PPI + 0x004 + (n*8))     // Disable channel group 0
//REGISTERS
#define NRF51_PPI_CHEN                  *(volatile uint32_t*) (NRF51_PPI + 0x500)             // Channel enable
#define NRF51_PPI_CHENSET               *(volatile uint32_t*) (NRF51_PPI + 0x504)             // Channel enable set
#define NRF51_PPI_CHENCLR               *(volatile uint32_t*) (NRF51_PPI + 0x508)             // Channel enable clear
#define NRF51_PPI_CH_EEP(n)             *(volatile uint32_t*) (NRF51_PPI + 0x510 + (n*8))     // Channel 0 event end-point
#define NRF51_PPI_CH_TEP(n)             *(volatile uint32_t*) (NRF51_PPI + 0x514 + (n*8))     // Channel 0 task end-point

////////////////////////////////////////////////////////////////////////////////////////////////////
/// UART ///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define NRF51_UART                     (volatile uint8_t*) 0x40002000

// TASKS
#define NRF51_UART_STARTRX             *(volatile uint32_t*) (NRF51_UART + 0x000)             //Start UART receiver
#define NRF51_UART_STOPRX              *(volatile uint32_t*) (NRF51_UART + 0x004)             //Stop UART receiver
#define NRF51_UART_STARTTX             *(volatile uint32_t*) (NRF51_UART + 0x008)             //Start UART transmitter
#define NRF51_UART_STOPTX              *(volatile uint32_t*) (NRF51_UART + 0x00C)             //Stop UART transmitter
// EVENTS
#define NRF51_UART_RXDRDY              *(volatile uint32_t*) (NRF51_UART + 0x108)             //Data received in RXD
#define NRF51_UART_TXDRDY              *(volatile uint32_t*) (NRF51_UART + 0x11C)             //Data sent from TXD
#define NRF51_UART_ERROR               *(volatile uint32_t*) (NRF51_UART + 0x124)             //Error detected
// REGISTERS
#define NRF51_UART_INTENSET            *(volatile uint32_t*) (NRF51_UART + 0x304)             //Interrupt enable set register
#define NRF51_UART_INTENCLR            *(volatile uint32_t*) (NRF51_UART + 0x308)             //Interrupt enable clear register
#define NRF51_UART_ERRORSRC            *(volatile uint32_t*) (NRF51_UART + 0x480)             //Error source
#define NRF51_UART_ENABLE              *(volatile uint32_t*) (NRF51_UART + 0x500)             //Enable and acquire IOs
#define NRF51_UART_PSELRTS             *(volatile uint32_t*) (NRF51_UART + 0x508)             //Pin select for RTS
#define NRF51_UART_PSELTXD             *(volatile uint32_t*) (NRF51_UART + 0x50C)             //Pin select for TXD
#define NRF51_UART_PSELCTS             *(volatile uint32_t*) (NRF51_UART + 0x510)             //Pin select for CTS
#define NRF51_UART_PSELRXD             *(volatile uint32_t*) (NRF51_UART + 0x514)             //Pin select for RXD
#define NRF51_UART_RXD                 *(volatile uint32_t*) (NRF51_UART + 0x518)             //RXD register
#define NRF51_UART_TXD                 *(volatile uint32_t*) (NRF51_UART + 0x51C)             //TXD register
#define NRF51_UART_BAUDRATE            *(volatile uint32_t*) (NRF51_UART + 0x524)             //Baud rate
#define NRF51_UART_CONFIG              *(volatile uint32_t*) (NRF51_UART + 0x56C)             //Configuration of parity and hardware flow control

#define NRF51_UART_CONFIG_HWFC_ENABLED 1   // Hardware flow control enabled
#define NRF51_UART_CONFIG_PARITY_EXCLUDE ~0b10   // Exclude parity bit
#define NRF51_UART_CONFIG_PARITY_INCLUDE 2 // Include parity bit
// C RW RXPARITY Configuration of parity bit used in reception
// MANUAL 0 9th bit received is saved in RXPARITY register
// AUTO 1 9th bit received is compared to a parity generated from the 8 first bits
// D RW TXPARITY Configuration of parity used in transmission
// MANUAL 0 9th bit transmitted is taken from the TXPARITY register
// AUTO 1 9th bit transmitted is parity bit generated from the 8 first bits


////////////////////////////////////////////////////////////////////////////////////////////////////
/// SPI ///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


#define NRF51_SPI_N(n)                 0x40003000+(0x1000*n)
// EVENTS
#define NRF51_SPI_READY(n)             *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x108)         //TXD byte sent and RXD byte received
// REGISTERS
#define NRF51_SPI_INTENSET(n)          *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x304)         //Interrupt enable set register
#define NRF51_SPI_INTENCLR(n)          *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x308)         //Interrupt enable clear register
#define NRF51_SPI_ENABLE(n)            *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x500)         //Enable SPI
#define NRF51_SPI_PSELSCK(n)           *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x508)         //Pin select for SCK
#define NRF51_SPI_PSELMOSI(n)          *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x50C)         //Pin select for MOSI
#define NRF51_SPI_PSELMISO(n)          *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x510)         //Pin select for MISO
#define NRF51_SPI_RXD(n)               *(volatile uint8_t*)  (NRF51_SPI_N(n) + 0x518)         //RXD register
#define NRF51_SPI_TXD(n)               *(volatile uint8_t*)  (NRF51_SPI_N(n) + 0x51C)         //TXD register
#define NRF51_SPI_FREQUENCY(n)         *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x524)         //SPI frequency


#define NRF51_SPI_FREQUENCY_125_K      0x02000000    // 125 kbps
#define NRF51_SPI_FREQUENCY_250K       0x04000000    // 250 kbps
#define NRF51_SPI_FREQUENCY_500K       0x08000000    // 500 kbps
#define NRF51_SPI_FREQUENCY_1M         0x10000000    // 1 Mbps
#define NRF51_SPI_FREQUENCY_2M         0x20000000    // 2 Mbps
#define NRF51_SPI_FREQUENCY_4M         0x40000000    // 4 Mbps
#define NRF51_SPI_FREQUENCY_8M         0x80000000    // 8 Mbps

#define NRF51_SPI_CONFIG(n)            *(volatile uint32_t*) (NRF51_SPI_N(n) + 0x554)         //Configuration register
#define NRF51_SPI_CONFIG_MSBFIRST ~1
#define NRF51_SPI_CONFIG_LSBFIRST 2
#define NRF51_SPI_CONFIG_ACTIVE_HIGH ~2
#define NRF51_SPI_CONFIG_ACTIVE_LOW 2
#define NRF51_SPI_CONFIG_CPHA_LEADING ~4
#define NRF51_SPI_CONFIG_CPHA_TRAILING 4


// ARMV v6 ARM Table B3-18 NVIC register summary
#define NRF51_NVIC_ISER *(volatile uint32_t*)0xE000E100                                       // Interrupt Set-Enable Register.
#define NRF51_NVIC_ICER *(volatile uint32_t*)0xE000E180                                       // Interrupt Clear-Enable Register
#define NRF51_NVIC_ISPR *(volatile uint32_t*)0xE000E200                                       // Interrupt Set-Pending Register
#define NRF51_NVIC_ICPR *(volatile uint32_t*)0xE000E280                                       // Interrupt Clear-Pending Register
#define NRF51_NVIC_IPRn(n) *(volatile uint32_t*)(0xE000E400+(4*n))                            // Interrupt Priority Registers



#endif /* __nRF51822_h */
