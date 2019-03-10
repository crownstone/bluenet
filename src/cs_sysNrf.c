/**
 * Author: Crownstone Team, Christopher Mason
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 14 Aug., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <stdint.h>

#include <ble/cs_Nordic.h>

/*
extern unsigned const long _stext;
extern unsigned const long _etext;
extern unsigned const long _sdata;
extern unsigned const long _edata;
extern unsigned const long _sbss;
extern unsigned const long _ebss;
extern unsigned const long _estack;
//extern unsigned const long __bss_start__;
//extern unsigned const long __bss_end__;
//extern unsigned const long __text_end__;
//extern unsigned const long __data_end__;
//extern unsigned const long __data_start__;
//extern void __init_array_start(void);
//extern void __init_array_end(void);

void unused_isr(void) {
}

//! Reset handler, sets clock frequency, handles product anomalies, initializes memory.
void ResetHandler(void);

void NMI_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void HardFault_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void MemoryManagement_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void BusFault_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void UsageFault_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void SVC_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void DebugMonitor_Handler(void) __attribute__ ((weak, alias("unused_isr")));
void PendSV_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void SysTick_Handler(void)	__attribute__ ((weak, alias("unused_isr")));


void POWER_CLOCK_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void RADIO_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));

//! Used for serial
void UART0_IRQHandler(void);

void SPI0_TWI0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SPI1_TWI1_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void NFCT_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void GPIOTE_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));

//! AD converter should be implemented.
void SAADC_IRQHandler(void);

// timer used by softdevice
void TIMER0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));

// timers used for ADC
void TIMER1_IRQHandler(void);
void TIMER2_IRQHandler(void);

// timer used for dimming
#if DIMMING==0
void TIMER4_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
#else
void TIMER4_IRQHandler(void);
#endif

void RTC0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void TEMP_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void RNG_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void ECB_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void CCM_AAR_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void WDT_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));

//! Real-time clock
void RTC1_IRQHandler(void);

void QDEC_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));

//! The LP comparator
void COMP_LPCOMP_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));

void SWI0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI1_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI2_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI3_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI4_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI5_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
//void TIMER2_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void TIMER3_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void PWM0_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void PDM_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void MWU_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void PWM1_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void PWM2_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void SPIM2_SPIS2_SPI2_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void RTC2_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void I2S_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));
void FPU_IRQHandler(void) __attribute__ ((weak, alias("unused_isr")));

__attribute__ ((section(".vectors"), used))
void (* const gVectors[])(void) =
{
	(void (*)(void))((unsigned long)&_estack),   // 0 ARM: Initial Stack Pointer
	ResetHandler,                                // 1 ARM: Initial Program Counter
	NMI_Handler,                                 // 2
	HardFault_Handler,
	MemoryManagement_Handler,
	BusFault_Handler,
	UsageFault_Handler,
	0,                                           // reserved
	0,
	0,
	0,                                           // 10
	SVC_Handler,
	DebugMonitor_Handler,
	0,
	PendSV_Handler,
	SysTick_Handler,

//! External Interrupts
	POWER_CLOCK_IRQHandler,
	RADIO_IRQHandler,
	UART0_IRQHandler,
	SPI0_TWI0_IRQHandler,
	SPI1_TWI1_IRQHandler,                        // 20
	NFCT_IRQHandler,
	GPIOTE_IRQHandler,
	SAADC_IRQHandler,
	TIMER0_IRQHandler,
	TIMER1_IRQHandler,
	TIMER2_IRQHandler,
	RTC0_IRQHandler,
	TEMP_IRQHandler,
	RNG_IRQHandler,
	ECB_IRQHandler,                              // 30
	CCM_AAR_IRQHandler,
	WDT_IRQHandler,
	RTC1_IRQHandler,
	QDEC_IRQHandler,
	COMP_LPCOMP_IRQHandler,
	SWI0_IRQHandler,
	SWI1_IRQHandler,
	SWI2_IRQHandler,
	SWI3_IRQHandler,
	SWI4_IRQHandler,                             // 40
	SWI5_IRQHandler,
	TIMER3_IRQHandler,
	TIMER4_IRQHandler,
	PWM0_IRQHandler,
	PDM_IRQHandler,
	MWU_IRQHandler,
	PWM1_IRQHandler,
	PWM2_IRQHandler,
	SPIM2_SPIS2_SPI2_IRQHandler,
	RTC2_IRQHandler,                             // 50
	I2S_IRQHandler,
	FPU_IRQHandler
};
*/

#ifdef NOT_USING_NORDIC_SYSTEMINIT

extern int main (void);

void SystemInit(void) {
	main();
}
#endif

#ifdef NOT_USING_NORDIC_RESET_HANDLER

__attribute__ ((section(".startup")))
void Reset_Handler(void) {

	//! Enable all RAM banks.
	//! See PAN_028_v1.6.pdf "16. POWER: RAMON reset value causes problems under certain conditions"
	//! Note: this was for the nRF51
//#ifdef NRF51
//	NRF_POWER->RAMON |= 0xF;
//#endif

	//! Enable Peripherals.
	//! See PAN_028_v1.6.pdf "26. System: Manual setup is required to enable use of peripherals"
	//! WARNING. This is only true for OLD hardware (check with ./scripts/hardware_version.sh)
	//! For new hardware this DISABLES for example the COMP_LPCOMP peripheral
	//! Note: this was for the nRF51
//#ifdef NRF51
//#if(HARDWARE_VERSION == 0x001D)
//	*(uint32_t *)0x40000504 = 0xC007FFDF;
//	*(uint32_t *)0x40006C18 = 0x00008000;
//#endif
//#endif

	//! start up crystal LF clock.
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
#if LOW_POWER_MODE==0
	/**
	 * The RFduino synthesizes the low frequency clock from the high frequency clock. There is no external crystal
	 * that can be used. It doesn't seem from the datasheets that there is a pin open for a crystal...
	 * Synthesizing the clock is of course not very energy efficient.
	 *
	 * Clock runs on 32768 Hz and is generated from the 16 MHz system clock
	 */
	//! start up crystal HF clock.
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(!NRF_CLOCK->EVENTS_HFCLKSTARTED)/** wait */;

	NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Synth;
#else
	//! Explicitly don't start the HF clock
	NRF_CLOCK->TASKS_HFCLKSTART = 0;

	//! TODO: make dependable on board

	//! Best option, but requires a 32kHz crystal on the pcb
//	NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos;

	//! Internal oscillator
	NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos;
#endif

	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while(!NRF_CLOCK->EVENTS_LFCLKSTARTED)/** wait */;
//	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

	/*
	 * There are two power modes in the nRF51, system ON, and system OFF. The former has two sub power modes. The
	 * first is called "Constant Latency", the second is called "Low Power". The latter is saving most of the
	 * power, the former keeps the CPU wakeup latency and automated task response at a minimum, but some resources
	 * will be kept active when the device is in sleep mode, such as the 16MHz clock.
	 */
#if LOW_POWER_MODE==0
	//! enable constant latency mode.
	NRF_POWER->TASKS_CONSTLAT = 1;
#else
	NRF_POWER->TASKS_LOWPWR = 1;
#endif

	/*
	uint32_t *src = (uint32_t*)&_etext;
	uint32_t *dest = (uint32_t*)&_sdata;

	//! copy data and clear bss
	while (dest < (uint32_t*)&_edata) *dest++ = *src++;
	while (dest < (uint32_t*)&_sbss) *dest++ = 0xdeadbeef;
	dest = (uint32_t*)&_sbss;
	while (dest < (uint32_t*)&_ebss) *dest++ = 0;
	while (dest < (uint32_t*)&dest) *dest++ = 0xdeadbeef;
	*/
	//! include libc functionality
	//__libc_init_array();

	main();
	while (1) ;

}
#endif
