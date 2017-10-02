/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/cs_Serial.h"

#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>

#include "app_util.h"
#include "nrf.h"

#include <ble/cs_Nordic.h>
#include "cfg/cs_Boards.h"

#ifdef INCLUDE_TIMESTAMPS
#include <drivers/cs_RTC.h>
#endif

static const uint32_t m_baudrates[UART_BAUD_TABLE_MAX_SIZE] = UART_BAUDRATE_DEVISORS_ARRAY;

/**
 * Configure the UART. Currently we set it on 38400 baud.
 */
void config_uart(uint8_t pinRx, uint8_t pinTx) {

#if SERIAL_VERBOSITY<SERIAL_NONE
	// Enable UART
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;

	// Configure UART pins
	NRF_UART0->PSELRXD = pinRx;
	NRF_UART0->PSELTXD = pinTx;

	//NRF_UART0->CONFIG = NRF_UART0->CONFIG_HWFC_ENABLED; // Do not enable hardware flow control.
	NRF_UART0->BAUDRATE = m_baudrates[UART_BAUD_38K4];
//	NRF_UART0->BAUDRATE = m_baudrates[UART_BAUD_230K4]; // Highest baudrate that still worked.
	NRF_UART0->TASKS_STARTTX = 1;
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
	NRF_UART0->EVENTS_TXDRDY = 0;
#else
	//! Disable UART
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos;

	NRF_UART0->TASKS_STOPRX = 1;
	NRF_UART0->TASKS_STOPTX = 1;
#endif
}

/**
 * Write an individual character to UART.
 */
//void write_uart(const char *str) {
//	int16_t len = strlen(str);
//	for(int i = 0; i < len; ++i) {
//		NRF_UART0->EVENTS_TXDRDY = 0;
//		NRF_UART0->TXD = (uint8_t)str[i];
//		while(NRF_UART0->EVENTS_TXDRDY != 1) {}
//	}
//}

/**
 * Read an individual character from UART.
 */
uint8_t read_uart() {
#if SERIAL_VERBOSITY<SERIAL_NONE
	while(NRF_UART0->EVENTS_RXDRDY != 1) {}
	NRF_UART0->EVENTS_RXDRDY = 0;
	return (uint8_t)NRF_UART0->RXD;
#endif
	return -1;
}

/**
 * A write function with a format specifier.
 */
int write(const char *str, ...) {
#if SERIAL_VERBOSITY<SERIAL_NONE
	char buffer[128];
	va_list ap;
	va_start(ap, str);
	int16_t len = vsprintf(NULL, str, ap);
	va_end(ap);

	if (len < 0) return len;

	//! if strings are small we do not need to allocate by malloc
	if (sizeof buffer >= len + 1UL) {
		va_start(ap, str);
		len = vsprintf(buffer, str, ap);
		va_end(ap);
		for(int i = 0; i < len; ++i) {
			NRF_UART0->EVENTS_TXDRDY = 0;
			NRF_UART0->TXD = (uint8_t)buffer[i];
			while(NRF_UART0->EVENTS_TXDRDY != 1) {}
		}
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
		for(int i = 0; i < len; ++i) {
			NRF_UART0->EVENTS_TXDRDY = 0;
			NRF_UART0->TXD = (uint8_t)p_buf[i];
			while(NRF_UART0->EVENTS_TXDRDY != 1) {}
		}
		free(p_buf);
	}
	return len;
#endif
	return 0;
}

#ifdef INCLUDE_TIMESTAMPS

int now() {
	return RTC::now();
}

#endif


