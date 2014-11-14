/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <drivers/serial.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "nRF51822.h"
#include "common/boards.h"

#define NRF51_UART_9600_BAUD  0x00275000UL
#define NRF51_UART_38400_BAUD 0x009D5000UL

/**
 * Configure the UART. Currently we set it on 38400 baud.
 */
void config_uart() {
	// Enable UART
	NRF51_UART_ENABLE = 0x04; 

	// Configure UART pins
	NRF51_UART_PSELRXD = PIN_RX;  
	NRF51_UART_PSELTXD = PIN_TX; 

	//NRF51_UART_CONFIG = NRF51_UART_CONFIG_HWFC_ENABLED; // do not enable hardware flow control.
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
	int16_t len = strlen(str);
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
	int16_t len = vsprintf(NULL, str, ap);
	va_end(ap);

	if (len < 0) return len;

	// if strings are small we do not need to allocate by malloc
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

