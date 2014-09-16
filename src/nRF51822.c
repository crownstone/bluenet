#include <stdint.h>
#include "nRF51822.h"

// Currently, very dirty set just RFDUINO here. We must make this of course more flexible and be able to compile
// smoothly for different modules created around the nRF51822 chip.
#define RFDUINO

extern unsigned const long _stext;
extern unsigned const long _etext;
extern unsigned const long _sdata;
extern unsigned const long _edata;
extern unsigned const long _sbss;
extern unsigned const long _ebss;
extern unsigned const long _estack;
extern unsigned const long __bss_start__;
extern unsigned const long __bss_end__;
extern unsigned const long __text_end__;
extern unsigned const long __data_end__;
extern unsigned const long __data_start__;
//extern void __init_array_start(void);
//extern void __init_array_end(void);
extern int main (void);
void ResetHandler(void);

void unused_isr(void)
{
}


void NMI_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void HardFault_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void SVC_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void PendSV_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void SysTick_Handler(void)	__attribute__ ((weak, alias("unused_isr")));
void POWER_CLOCK_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void RADIO_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void UART0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SPI0_TWI0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SPI1_TWI1_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void GPIOTE_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void ADC_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void TIMER0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void TIMER1_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void TIMER2_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void RTC0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void TEMP_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void RNG_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void ECB_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void CCM_AAR_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void WDT_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void RTC1_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void QDEC_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void WUCOMP_COMP_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI0_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI1_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI2_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI3_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI4_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));
void SWI5_IRQHandler(void)	__attribute__ ((weak, alias("unused_isr")));

__attribute__ ((section(".vectors"), used))
void (* const gVectors[])(void) =
{        
	(void (*)(void))((unsigned long)&_estack),	//  0 ARM: Initial Stack Pointer
	ResetHandler,					//  1 ARM: Initial Program Counter
	NMI_Handler,
	HardFault_Handler,
	0,              // reserved
	0,
	0,
	0,
	0,
	0,
	0,
	SVC_Handler,
	0,
	0,
	PendSV_Handler,
	SysTick_Handler,

	/* External Interrupts */
	POWER_CLOCK_IRQHandler,
	RADIO_IRQHandler,
	UART0_IRQHandler,
	SPI0_TWI0_IRQHandler,
	SPI1_TWI1_IRQHandler,
	0,
	GPIOTE_IRQHandler,
	ADC_IRQHandler,
	TIMER0_IRQHandler,
	TIMER1_IRQHandler,
	TIMER2_IRQHandler,
	RTC0_IRQHandler,
	TEMP_IRQHandler,
	RNG_IRQHandler,
	ECB_IRQHandler,
	CCM_AAR_IRQHandler,
	WDT_IRQHandler,
	RTC1_IRQHandler,
	QDEC_IRQHandler,
	WUCOMP_COMP_IRQHandler,
	SWI0_IRQHandler,
	SWI1_IRQHandler,
	SWI2_IRQHandler,
	SWI3_IRQHandler,
	SWI4_IRQHandler,
	SWI5_IRQHandler,
	0,
	0,
	0,
	0,
	0,
	0,

};

__attribute__ ((section(".startup")))
void ResetHandler(void) {

	// Enable all RAM banks. 
	// See PAN_028_v1.6.pdf "16. POWER: RAMON reset value causes problems under certain conditions"
	NRF51_POWER_RAMON |= 0xF;

	// Enable Peripherals. 
	// See PAN_028_v1.6.pdf "25. System: Manual setup is required to enable use of peripherals"
	*(uint32_t *)0x40000504 = 0xC007FFDF;
	*(uint32_t *)0x40006C18 = 0x00008000;

	// start up crystal HF clock.
	NRF51_CLOCK_HFCLKSTART = 1;
	while(!NRF51_CLOCK_HFCLKSTARTED) /* wait */;

	// start up crystal LF clock.
#ifdef RFDUINO
	/**
	 * The RFduino synthesizes the low frequency clock from the high frequency clock. There is no external crystal 
	 * that can be used. It doesn't seem from the datasheets that there is a pin open for a crystal... 
	 * Synthesizing the clock is of course not very energy efficient. 
	 */
	NRF51_CLOCK_LFCLKSRC = NRF51_CLOCK_LFCLKSRC_SYNTH;
#else
	NRF51_CLOCK_LFCLKSRC = NRF51_CLOCK_LFCLKSRC_XTAL;
#endif
	NRF51_CLOCK_LFCLKSTART = 1;
	while(!NRF51_CLOCK_LFCLKSTARTED) /* wait */;

	/*
	 * There are two power modes in the nRF51, system ON, and system OFF. The former has two sub power modes. The 
	 * first is called "Constant Latency", the second is called "Low Power". The latter is saving most of the 
	 * power, the former keeps the CPU wakeup latency and automated task response at a minimum, but some resources
	 * will be kept active when the device is in sleep mode, such as the 16MHz clock.
	 */
	// enable constant latency mode.
	NRF51_POWER_CONSTLAT = 1;

	uint32_t *src = &_etext;
	uint32_t *dest = &_sdata;

	// copy data and clear bss
	while (dest < &_edata) *dest++ = *src++;
	while (dest < &_sbss) *dest++ = 0xdeadbeef;
	dest = &_sbss;
	while (dest < &_ebss) *dest++ = 0;
	while (dest < &dest) *dest++ = 0xdeadbeef;

	// include libc functionality
	__libc_init_array();

	main();
	while (1) ;

}


