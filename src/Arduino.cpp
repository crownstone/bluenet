#include "Arduino.h"

#include "Debug.h"

#if NRF51_USE_SOFTDEVICE == 1
#warning "SoftDevice is used... Make sure SoftDevice headers are available"
#include "nrf_soc.h"
#include "nrf51.h"
#include "nrf_sdm.h"
#endif


void enable_irq(uint32_t irqn, irq_prio_t prio) {
#if NRF51_USE_SOFTDEVICE == 1
	uint32_t result __attribute__((unused)) = 0;
	uint8_t sden;
	sd_softdevice_is_enabled(&sden);
	if (sden) {
		result = sd_nvic_SetPriority((IRQn_Type)irqn, (nrf_app_irq_priority_t)prio);
		if (result != 0) {
			while(1) {}
		}
		result = sd_nvic_EnableIRQ((IRQn_Type)irqn);
		if (result != 0) {
			while(1) {}
		}
	} else {
		NRF51_NVIC_IPRn(irqn) = prio;
		NRF51_NVIC_ISER = 1 << irqn;
	}
#else /* NRF51_USE_SOFTDEVICE */
	NRF51_NVIC_IPRn(irqn) = prio;
	NRF51_NVIC_ISER = 1 << irqn;
#endif /* NRF51_USE_SOFTDEVICE */

}

/**
 * The TIMER0, TIMER1, and TIMER2 run on the high-frequency clock source. They include a four bit prescaler 1/X^2 that 
 * can bring the frequency down. The RTC0 and RTC1 are low power timers on the low-frequency clock source. They feature a 
 * 24bit counter and a 12 bit 1/X prescaler.
 *
 * The RTC0 is a real-time counter which is used by the SoftDevice when in use. Hence, we use the RTC1 real-time 
 * counter here.
 */
void tick_init() {

	// set interrupts for the real-time clock
	enable_irq(NRF51_RTC1_IRQn, IRQ_APP_PRIORITY_LOW);

	//
	// The low-frequency clock runs on 32768 kHz. It can be externally driven or synthesized. With prescaling 
	// factor of 32, this is around 1 kHz.
	//
	NRF51_RTC1_PRESCALER = 32; // 1kHz

	NRF51_RTC1_EVTENSET = NRF51_RTC1_EVTEN_OVERFLOW;
	NRF51_RTC1_INTENSET = NRF51_RTC1_EVTEN_OVERFLOW | NRF51_RTC1_EVTEN_COMPARE0;

	NRF51_RTC1_START = 1;

	// set interrupts for the high frequence timer
	enable_irq(NRF51_TIMER1_IRQn, IRQ_APP_PRIORITY_LOW);

	//
	// Prescaling with a factor of 2^4 here, makes the timer 1MHz. This timer is used in the delay task.
	//
	NRF51_TIMER1_SHORTS = NRF51_TIMER_SHORTS_COMPARE0_CLEAR | NRF51_TIMER_SHORTS_COMPARE0_STOP;
	NRF51_TIMER1_INTENSET = NRF51_EVENT_INTERRUPT_N(NRF51_TIMER1, NRF51_TIMER1_COMPARE_0);
	NRF51_TIMER1_STOP = 1;
	NRF51_TIMER1_MODE = NRF51_TIMER1_MODE_TIMER;
	NRF51_TIMER1_COMPARE_0 = 0;
	NRF51_TIMER1_PRESCALER = 4;
	NRF51_TIMER1_BITMODE = NRF51_TIMER1_BITMODE_16_BIT;
}


extern "C" {

	extern void * sbrk(int incr);

	volatile bool delaying = false;
	void TIMER1_IRQHandler(void) {
		if (NRF51_TIMER1_COMPARE_0) {
			NRF51_TIMER1_COMPARE_0 = 0;
			delaying = false;
		}
	}

	extern volatile uint32_t overflow_counter = 0;

	void RTC1_IRQHandler () {
		if (NRF51_RTC1_COMPARE_0) {
			NRF51_RTC1_COMPARE_0 = 0;
			delaying = false;
		}
		if (NRF51_RTC1_OVRFLW) {
			NRF51_RTC1_OVRFLW = 0;
			overflow_counter++;
		}
		void* sp;
		asm("mov %0, sp" : "=r"(sp) : : );
		if (sp <= sbrk(0)) {
			NRF51_CRASH("Stack overflow.");
		}
	}

}

void pinMode(uint8_t pin, uint8_t mode) {
	if (mode == INPUT) {
		NRF51_GPIO_DIRCLR = 1 << pin;
	} else if (mode == INPUT_PULLUP) {
		NRF51_GPIO_PIN_CNF(pin) |= 1 << 3;
	} else if (mode == OUTPUT) {
		NRF51_GPIO_DIRSET = 1 << pin;
	}
}

int analogRead(uint8_t pin) {
	return 700; // TODO
}

void ppi_assign(uint8_t channel, volatile uint32_t* event, volatile uint32_t* task) {
#if NRF51_USE_SOFTDEVICE == 1
	uint8_t sden;
	sd_softdevice_is_enabled(&sden);
	if (!sden) {
		NRF51_PPI_CH_EEP(channel) = (uint32_t)event;
		NRF51_PPI_CH_TEP(channel) = (uint32_t)task;
	} else {
		volatile uint32_t error;
		error = sd_ppi_channel_assign(channel, event, task);
		if (error != 0) while(1) {}
	}
#else /* NRF51_USE_SOFTDEVICE */
	NRF51_PPI_CH_EEP(channel) = (uint32_t)event;
	NRF51_PPI_CH_TEP(channel) = (uint32_t)task;
#endif /* NRF51_USE_SOFTDEVICE */
}

void ppi_enable(uint32_t channels) {
#if NRF51_USE_SOFTDEVICE == 1
	uint8_t sden;
	sd_softdevice_is_enabled(&sden);
	if (!sden) {
		NRF51_PPI_CHENSET = channels;
	} else {
		volatile uint32_t error;
		error = sd_ppi_channel_enable_set( channels );
		if (error != 0) while(1) {}
	}
#else
	NRF51_PPI_CHENSET = channels;
#endif
}

void ppi_disable(uint32_t channels) {
#if NRF51_USE_SOFTDEVICE == 1
	uint8_t sden;
	sd_softdevice_is_enabled(&sden);
	if (!sden) {
		NRF51_PPI_CHENCLR = channels;
	} else {
		volatile uint32_t error;
		error = sd_ppi_channel_enable_clr( channels );
		if (error != 0) while(1) {}
	}
#else
	NRF51_PPI_CHENCLR = channels;
#endif
}

int8_t first_free_channel() {
	uint8_t i; int8_t r = -1;
	for (i = 0; i < 8; ++i) {
		if (!(NRF_PPI->CHEN & (1 << i))) {
			r = (int8_t)i;
			return r;
		}
	}
	return r;
}

// A struct to store the pin numbers for a channel.
uint8_t pwm_pin_channel[31] = {};

void pwm_init(uint8_t pin, uint8_t channel) {
	// after pwm_example/main.c and nrf_gpiote.h from nordic SDK.
	
	// set interrupts for the high frequence timer 2
	enable_irq(NRF51_TIMER2_IRQn, IRQ_APP_PRIORITY_LOW);

	// BITMODE only 16-bit? http://forum.rfduino.com/index.php?topic=155.0

	// set up the timer.  we'll use two compare registers, one to turn the pin on, and the other to turn it off.
	// the value of the first compare register will determine the duty cycle of the PWM.
	// TODO: choose which timer compare registers based on channel num so we can have more than one channel of PWM.
	NRF51_TIMER2_STOP = 1;
	NRF51_TIMER2_MODE = NRF51_TIMER2_MODE_TIMER;
	NRF51_TIMER2_PRESCALER = 9;
	NRF51_TIMER2_BITMODE = NRF51_TIMER2_BITMODE_16_BIT;
	NRF51_TIMER2_SHORTS = NRF51_TIMER_SHORTS_COMPARE1_CLEAR; // set timer2 compare1 to clear timer when it's reached.
	NRF51_TIMER2_CC_0 = 100; // when reaching the value in this compare register, the pin is turned on
	NRF51_TIMER2_CC_1 = 255; // when reaching the value in this compare register, the pin is turned off

	// set pin to output, and clear it
	NRF51_GPIO_DIR_OUTPUT(pin);
	NRF51_GPIO_OUTCLR = 1 << pin;

	// configure the GPIO tasks and events to toggle the pin.
	// I believe this odd sequence with the NOPs is to work around bugs described in PAN 1.5.
	NRF51_GPIOTE_CONFIG_N(channel) = NRF51_GPIOTE_CONFIG_MODE_TASK | NRF51_GPIOTE_CONFIG_POLARITY_LOTOHI | NRF51_GPIOTE_CONFIG_PSEL_NONE;

	// 3 NOPs required to set config.
	__asm("NOP;");
	__asm("NOP;");
	__asm("NOP;");

	// what do we do here? 
	NRF51_GPIOTE_OUT_N(channel) = 1;

	// configure the given pin as a task, where each invocation of the task flips (toggles) the state of the pin, and with initial value hi.
	NRF51_GPIOTE_CONFIG_N(channel) = NRF51_GPIOTE_CONFIG_MODE_TASK | NRF51_GPIOTE_CONFIG_POLARITY_TOGGLE | NRF51_GPIOTE_CONFIG_PSEL(pin) | NRF51_GPIOTE_CONFIG_OUTINIT_HIGH;

	// 3 NOPs required to set config.
	__asm("NOP;");
	__asm("NOP;");
	__asm("NOP;");

	// Configure the PPI to connect the firing of the timer2 compare 0 event to the GPIO task that flips the pin.
	ppi_assign(channel*2, &NRF51_TIMER2_COMPARE_0, &NRF51_GPIOTE_OUT_0);

	// Also configure the PPI to connect the firing of the timer2 compare 1 event to the GPIO task that flips the pin.
	ppi_assign(channel*2+1, &NRF51_TIMER2_COMPARE_1, &NRF51_GPIOTE_OUT_0);

	// enable those two PPI channels.
	ppi_enable( (1 << (channel * 2)) | (1 << (channel * 2 + 1)));

	// keep track of which channel we used for this pin.
	pwm_pin_channel[pin] = channel + 1;  // so 0 is uninitialized, not really nice trick

	NRF51_GPIOTE_CONFIG_N(channel) &= ~NRF51_GPIOTE_CONFIG_MODE_TASK;

//	NRF51_GPIO_OUTCLR = 1 << pin;
//	NRF51_TIMER2_START = 1;
}



void analogWrite(uint8_t pin, uint8_t val) {
//	__asm("BKPT");
	uint8_t channel = pwm_pin_channel[pin];
	if (channel == 0) {
		// throw?
		while(1) {}
	}
	channel -= 1;

	if ((val == 0) || (val == 255)) {
		NRF51_TIMER2_STOP = 1;
		ppi_disable( (1 << (channel * 2)) | (1 << (channel * 2 + 1)));
		if (val == 255) {
			NRF51_GPIO_OUTSET = 1 << pin;
		} else {
			NRF51_GPIO_OUTCLR = 1 << pin;
		}
	} else {
		/**
		 * The timer will be started. Before the pin is cleared. Then the timer is run. As soon as it reaches 
		 * "val", it will be set to one. 
		 */
		NRF51_GPIOTE_CONFIG_N(channel) &= ~NRF51_GPIOTE_CONFIG_MODE_TASK;
		ppi_enable( (1 << (channel * 2)) | (1 << (channel * 2 + 1)));
		NRF51_GPIO_OUTCLR = 1 << pin;
	//	NRF51_GPIOTE_CONFIG_N(channel) |= NRF51_GPIOTE_CONFIG_MODE_TASK;
		NRF51_TIMER2_CC_0 = val;
		NRF51_TIMER2_START = 1;
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
		__asm("NOP;");
	}
}

void yield() {}
