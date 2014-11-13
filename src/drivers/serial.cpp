/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 10 Oct., 2014
 * License: LGPLv3+
 */

#include <drivers/serial.h>
#include <drivers/gpio_api.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "nRF51822.h"
#include "common/boards.h"

#define NRF51_UART_9600_BAUD  0x00275000UL
#define NRF51_UART_38400_BAUD 0x009D5000UL

void config_uart() {
	gpio_t uart_out;
	gpio_t uart_in;

	gpio_init_in(&uart_in, RX_PIN_NUMBER);
	gpio_init_out(&uart_out, TX_PIN_NUMBER);

	//NRF51_GPIO_DIR_OUTPUT(17); // set pins to output
	NRF51_GPIO_PIN_CNF(16) = NRF51_GPIO_PIN_CNF_PULL_DISABLED;
	//NRF51_GPIO_DIR_INPUT(16);
	//NRF51_GPIO_DIRSET = 3<<8;
	NRF51_UART_ENABLE = 0x04; // 0b00000100

	// Configure UART pins:    GPIO   UART
	NRF51_UART_PSELRXD = PIN_RX;  	// P0.11  RXD //16
	NRF51_UART_PSELTXD = PIN_TX;   	// P0.09  TXD //17
	//NRF51_UART_PSELRTS = 18;   	// P0.08  RTS //18
	//NRF51_UART_PSELCTS = 19;  	// P0.10  CTS //19

	//NRF51_UART_CONFIG = NRF51_UART_CONFIG_HWFC_ENABLED; // enable hardware flow control.
	NRF51_UART_BAUDRATE = NRF51_UART_38400_BAUD;
	NRF51_UART_STARTTX = 1;
	NRF51_UART_STARTRX = 1;
	NRF51_UART_RXDRDY = 0;
	NRF51_UART_TXDRDY = 0;
}

/**
 * Write an individual character to UART.
 */
void write_uart(const char *str) {
	uint8_t len = strlen(str);
	for(int i = 0; i < len; ++i) {
		NRF51_UART_TXD = (uint8_t)str[i];
		while(NRF51_UART_TXDRDY != 1) {}
		NRF51_UART_TXDRDY = 0;
	}
}

/**
 * Read an individual character from UART.
 */
uint8_t read_uart() {
	while(NRF51_UART_RXDRDY != 1) {}
	NRF51_UART_RXDRDY = 0;
	return (uint8_t)NRF51_UART_RXD;
}

/**
 * A write function with a format specifier.
 */
int write(const char *str, ...) {
	char buffer[128];
	va_list ap;
	va_start(ap, str);
	uint16_t len = vsprintf(NULL, str, ap);
	va_end(ap);

	if (len < 0) return len;

	
	if (sizeof buffer >= len + 1UL) {
		va_start(ap, str);
		len = vsprintf(buffer, str, ap);
		for(int i = 0; i < len; ++i) {
			NRF51_UART_TXD = (uint8_t)buffer[i];
			while(NRF51_UART_TXDRDY != 1) {}
			NRF51_UART_TXDRDY = 0;
		}
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
		for(int i = 0; i < len; ++i) {
			NRF51_UART_TXD = (uint8_t)p_buf[i];
			while(NRF51_UART_TXDRDY != 1) {}
			NRF51_UART_TXDRDY = 0;
		}
		free(p_buf);
	}
	return len;
}

