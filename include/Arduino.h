#ifndef _Arduino_h
#define _Arduino_h

#include "WProgram.h"
#include "nRF51822.h"

#ifdef __cplusplus
extern "C" {
#endif


// from core_pins.h
// these give me the willies:
#define HIGH		1
#define LOW		0
#define INPUT		0
#define OUTPUT		1
#define INPUT_PULLUP	2
#define LSBFIRST	0
#define MSBFIRST	1
#define _BV(n)		(1<<(n))
#define CHANGE		4
#define FALLING		2
#define RISING		3

typedef enum {
    IRQ_APP_PRIORITY_HIGH = 1,
    IRQ_APP_PRIORITY_LOW = 3
} irq_prio_t;

void enable_irq(uint32_t irqn, irq_prio_t prio);

void pinMode(uint8_t pin, uint8_t mode);

static inline void digitalWrite(uint8_t pin, uint8_t val) __attribute__((always_inline, unused));
static inline void digitalWrite(uint8_t pin, uint8_t val) {
    if (val) {
        NRF51_GPIO_OUTSET = 1 << pin;
    } else {
        NRF51_GPIO_OUTCLR = 1 << pin;
    }
}

#define A0	0
#define A1	1
#define A2	2
#define A3	3
#define A4	4
#define A5	5
#define A6	6
#define A7	7
#define A8	8


int analogRead(uint8_t pin);

void pwm_init(uint8_t pin, uint8_t channel);
void analogWrite(uint8_t pin, uint8_t val);

static inline int digitalRead(uint8_t pin) __attribute__((always_inline, unused));
static inline int digitalRead(uint8_t pin) {
    return NRF51_GPIO_IN & 1 << pin;
}

//static inline void delay(uint32_t ms) __attribute__((always_inline, unused));
//static inline void delay(uint32_t ms) {
//    nrf_delay_ms(ms);
//}
//
//static inline void delayMicroseconds(uint32_t us) __attribute__((always_inline, unused));
//static inline void delayMicroseconds(uint32_t us) {
//    nrf_delay_us(us);
//}

extern "C" {
extern volatile bool delaying;
}

static inline void delayMicroseconds(uint32_t us) __attribute__((always_inline, unused));
void delayMicroseconds(uint32_t us) {
    if (us < 4) { // soft device interrupt latency is quoted as 3.2 us in s110 soft device specification.
        while(--us) {
            asm(" NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t"
                    " NOP\n\t");
        };
        return;
    } else if (us < 10000) {
        delaying = true;
        NRF51_TIMER1_CLEAR = 1;
        NRF51_TIMER1_CC_0 = us-4;
        NRF51_TIMER1_COMPARE_0 = 0;
        NRF51_TIMER1_START = 1;
        while(delaying) {
            asm("WFI");
        }
        NRF51_TIMER1_STOP = 1;
    } else {
        delaying = true;
        uint32_t now = NRF51_RTC1_COUNTER;
        NRF51_RTC1_CC_0 = (now + (us / 1000)) % 0xFFFFFF;
        NRF51_RTC1_EVTENSET = NRF51_RTC1_EVTEN_COMPARE0;
        while(delaying) {
            asm("WFI");
        }
        NRF51_RTC1_EVTENCLR = NRF51_RTC1_EVTEN_COMPARE0;
    }


}
static inline void delay(uint32_t ms) __attribute__((always_inline, unused));
void delay(uint32_t ms) {
    delayMicroseconds(ms*1000);
}


void tick_init();

extern volatile uint32_t overflow_counter;

static inline uint32_t millis() __attribute__((always_inline, unused));
static inline uint32_t millis() {
    return NRF51_RTC1_COUNTER;
}


// TODO these should go somewhere else
typedef uint8_t boolean;
typedef uint8_t byte;

void yield();

#ifdef __cplusplus
}
#endif

#endif /* _Arduino_h */