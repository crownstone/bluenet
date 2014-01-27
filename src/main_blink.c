#include <string.h>
#include "nRF51822.h"

// P0.21-3 are red, green, blue LEDs on development dongle (PCA10000)
#define PIN_LED 21
// This is for my custom board:
//#define PIN_LED 30

int main(void) {

    NRF51_GPIO_DIRSET = 1 << PIN_LED; // set pins to output
    NRF51_GPIO_OUTSET = 1 << PIN_LED; // set pins high -- LED off
    NRF51_GPIO_OUTCLR = 1 << PIN_LED; // set red led on.

    NRF51_CLOCK_LFCLKSRC = NRF51_CLOCK_LFCLKSRC_XTAL;
	
	NRF51_RTC1_PRESCALER = 327; // 100 Hz
    NRF51_RTC1_START = 1;
	#define NRF51_RTC1_COUNTER_HZ 100
	
	while(NRF51_RTC1_COUNTER < NRF51_RTC1_COUNTER_HZ * 5) /* wait */;

    NRF51_GPIO_DIRSET = 3<<8;
    NRF51_UART_ENABLE = 0b100;
	
	// Configure UART pins:    GPIO   UART
    NRF51_UART_PSELRTS = 8;   	// P0.08  RTS
    NRF51_UART_PSELTXD = 9;   	// P0.09  TXD
    NRF51_UART_PSELCTS = 10;  	// P0.10  CTS
    NRF51_UART_PSELRXD = 11;  	// P0.11  RXD

    NRF51_UART_CONFIG = NRF51_UART_CONFIG_HWFC_ENABLED; // enable hardware flow control.
    NRF51_UART_BAUDRATE = 0x009D5000; // 38400
    NRF51_UART_STARTTX = 1;
	
	unsigned char* hello = "Hello, world.\n\r";
	
	uint8_t len = strlen(hello);

    NRF51_GPIO_OUTSET = 1 << PIN_LED; // set pin low - red LED off.

	uint32_t counter = NRF51_RTC1_COUNTER;
	uint8_t on = 1;
	while(1) {
		while(NRF51_RTC1_COUNTER - counter < (NRF51_RTC1_COUNTER_HZ/2)) /* wait */;
		on = !on;
		if (on) {
	            NRF51_GPIO_OUTSET = 1 << PIN_LED;
		} else {
        	    NRF51_GPIO_OUTCLR = 1 << PIN_LED;
		}

//		for(int i = 0; i < len; ++i) {
//            NRF51_UART_TXD = (uint8_t)hello[i];
//			while(!NRF51_UART_TXDRDY) /* wait */;
//            NRF51_UART_TXDRDY = 0;
//		}
		
		counter = NRF51_RTC1_COUNTER;
	}
	
}

