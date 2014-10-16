#include <serial.h>
#include <cstring>
#include "nRF51822.h"

#define NRF51_UART_9600_BAUD  0x00275000UL
#define NRF51_UART_38400_BAUD 0x009D5000UL

void config_uart() {
	NRF51_GPIO_DIR_OUTPUT(17); // set pins to output
	NRF51_GPIO_PIN_CNF(16) = NRF51_GPIO_PIN_CNF_PULL_DISABLED;
	NRF51_GPIO_DIR_INPUT(16);
	//NRF51_GPIO_DIRSET = 3<<8;
	NRF51_UART_ENABLE = 0x04; // 0b00000100

	// Configure UART pins:    GPIO   UART
	NRF51_UART_PSELRXD = 16;  	// P0.11  RXD //16
	NRF51_UART_PSELTXD = 17;   	// P0.09  TXD //17
	//NRF51_UART_PSELRTS = 18;   	// P0.08  RTS //18
	//NRF51_UART_PSELCTS = 19;  	// P0.10  CTS //19

	//NRF51_UART_CONFIG = NRF51_UART_CONFIG_HWFC_ENABLED; // enable hardware flow control.
	NRF51_UART_BAUDRATE = NRF51_UART_38400_BAUD;
	NRF51_UART_STARTTX = 1;
	NRF51_UART_STARTRX = 1;
	NRF51_UART_RXDRDY = 0;
	NRF51_UART_TXDRDY = 0;
}

void write(const char *str) {
	uint8_t len = strlen(str);
	for(int i = 0; i < len; ++i) {
		NRF51_UART_TXD = (uint8_t)str[i];
		while(NRF51_UART_TXDRDY != 1) /* wait */;
		NRF51_UART_TXDRDY = 0;
	}
}



