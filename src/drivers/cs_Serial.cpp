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

#include "util/cs_BleError.h"
#include <app_util_platform.h>

#include "util/cs_Utils.h"
#include "protocol/cs_UartProtocol.h"

#include "ble/cs_Nordic.h"
#include "cfg/cs_Boards.h"

#include "events/cs_EventDispatcher.h"

#ifdef INCLUDE_TIMESTAMPS
#include <drivers/cs_RTC.h>
#endif

// Define test pin to enable gpio debug.
//#define TEST_PIN 20


/**
 * Configure the UART. Currently we set it on 38400 baud.
 */
void config_uart(uint8_t pinRx, uint8_t pinTx) {
#ifdef TEST_PIN
    nrf_gpio_cfg_output(TEST_PIN);
#endif

#if SERIAL_VERBOSITY<SERIAL_NONE
	// Enable UART
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;

	// Enable interrupt
	uint32_t err_code;
	err_code = sd_nvic_SetPriority(UARTE0_UART0_IRQn, APP_IRQ_PRIORITY_MID);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(UARTE0_UART0_IRQn);
	APP_ERROR_CHECK(err_code);

	// Enable RX ready interrupts
	NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Msk;

	// Configure UART pins
	NRF_UART0->PSELRXD = pinRx;
	NRF_UART0->PSELTXD = pinTx;

	// Init parser
	UartProtocol::getInstance().init();

	//NRF_UART0->CONFIG = NRF_UART0->CONFIG_HWFC_ENABLED; // Do not enable hardware flow control.
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud38400;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud57600;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud76800;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud115200;
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud230400; // Highest baudrate that still worked.
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

inline void _writeByte(uint8_t val) {
#if SERIAL_VERBOSITY<SERIAL_READ_ONLY
	NRF_UART0->EVENTS_TXDRDY = 0;
	NRF_UART0->TXD = val;
	while(NRF_UART0->EVENTS_TXDRDY != 1) {}
#endif
}

/**
 * A write function with a format specifier.
 */
int write(const char *str, ...) {
#if SERIAL_VERBOSITY<SERIAL_BYTE_PROTOCOL_ONLY
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
//		writeBytes((uint8_t*)buffer, len);
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)buffer, len);
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
//		writeBytes((uint8_t*)p_buf, len);
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)p_buf, len);
		free(p_buf);
	}
	return len;
#endif
	return 0;
}

// TODO: use uart class for this.
void writeBytes(uint8_t* data, const uint16_t size) {
#if SERIAL_VERBOSITY<SERIAL_READ_ONLY
	for(int i = 0; i < size; ++i) {
		uint8_t val = (uint8_t)data[i];
		// Escape when necessary
		switch (val) {
			case UART_START_BYTE:
			case UART_ESCAPE_BYTE:
				_writeByte(UART_ESCAPE_BYTE);
				val ^= UART_ESCAPE_FLIP_MASK;
				break;
		}
		_writeByte(val);
	}
#endif
}

// TODO: use uart class for this.
void writeStartByte() {
#if SERIAL_VERBOSITY<SERIAL_READ_ONLY
	_writeByte(UART_START_BYTE);
#endif
}

#ifdef INCLUDE_TIMESTAMPS

int now() {
	return RTC::now();
}

#endif


static uint8_t readByte;

// UART interrupt handler
extern "C" void UART0_IRQHandler(void) {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	readByte = (uint8_t)NRF_UART0->RXD;
	UartProtocol::getInstance().onRead(readByte);

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	// Clear event after reading the data: new data may be written to RXD immediately.
	NRF_UART0->EVENTS_RXDRDY = 0;
}
