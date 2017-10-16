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

	//NRF_UART0->CONFIG = NRF_UART0->CONFIG_HWFC_ENABLED; // Do not enable hardware flow control.
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud38400;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud230400; // Highest baudrate that still worked.
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
 * A write function with a format specifier.
 */
int write(const char *str, ...) {
#if SERIAL_VERBOSITY<SERIAL_READ_ONLY
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


static uint8_t readByte;
static bool readBusy = false;

static void onByteRead(void * data, uint16_t size) {
	uint16_t event = 0;
	switch (readByte) {
	case 99: // c
		event = EVT_TOGGLE_LOG_CURRENT;
		break;
	case 70: // F
		write("Paid respect\r\n");
	case 102: // f
		event = EVT_TOGGLE_LOG_FILTERED_CURRENT;
		break;
	case 112: // p
		event = EVT_TOGGLE_LOG_POWER;
		break;
	case 114: // r
		event = EVT_CMD_RESET;
		break;
	case 118: // v
		event = EVT_TOGGLE_LOG_VOLTAGE;
		break;
	default:
		readBusy = false;
		return;
	}

	EventDispatcher::getInstance().dispatch(event);
	readBusy = false;
}

// UART interrupt handler
extern "C" void UART0_IRQHandler(void) {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	readByte = (uint8_t)NRF_UART0->RXD;
	write("read: %u\r\n", readByte);

	if (!readBusy) {
		readBusy = true;
		// Decouple callback from uart interrupt handler, and put it on app scheduler instead
		uint32_t errorCode = app_sched_event_put(&readByte, sizeof(readByte), onByteRead);
		APP_ERROR_CHECK(errorCode);
	}

	// Clear event after reading the data: new data may be written to RXD immediately.
	NRF_UART0->EVENTS_RXDRDY = 0;
}
