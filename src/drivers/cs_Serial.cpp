/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
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

// Define test pins to enable gpio debug.
//#define RX_PIN      20
//#define ERROR_PIN   22
//#define TIMEOUT_PIN 23
//#define INIT_UART_PIN 20
//#define INIT_TX_PIN   22
//#define INIT_RX_PIN   23

static uint8_t _pinRx = 0;
static uint8_t _pinTx = 0;
static bool _initialized = false;
static bool _initializedUart = false;
static bool _initializedRx = false;
static bool _initializedTx = false;
static serial_enable_t _state = SERIAL_ENABLE_NONE;

void serial_config(uint8_t pinRx, uint8_t pinTx) {
	_pinRx = pinRx;
	_pinTx = pinTx;
}

// Initializes anything but the UART peripheral.
// Only to be called once.
void init() {
	if (_initialized) {
		return;
	}
	_initialized = true;

#ifdef RX_PIN
	nrf_gpio_cfg_output(RX_PIN);
#endif
#ifdef ERROR_PIN
	nrf_gpio_cfg_output(ERROR_PIN);
#endif
#ifdef TIMEOUT_PIN
	nrf_gpio_cfg_output(TIMEOUT_PIN);
#endif
#ifdef INIT_UART_PIN
	nrf_gpio_cfg_output(INIT_UART_PIN);
#endif
#ifdef INIT_TX_PIN
	nrf_gpio_cfg_output(INIT_TX_PIN);
#endif
#ifdef INIT_RX_PIN
	nrf_gpio_cfg_output(INIT_RX_PIN);
#endif

	// Init parser
	UartProtocol::getInstance().init();

	// Enable interrupt handling
	uint32_t err_code;
	err_code = sd_nvic_SetPriority(UARTE0_UART0_IRQn, APP_IRQ_PRIORITY_MID);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(UARTE0_UART0_IRQn);
	APP_ERROR_CHECK(err_code);
}

// Initializes the UART peripheral, and enables interrupt.
void init_uart() {
	if (_initializedUart) {
		return;
	}
	_initializedUart = true;
#ifdef INIT_UART_PIN
	nrf_gpio_pin_toggle(INIT_UART_PIN);
#endif

	// Configure UART pins
	NRF_UART0->PSELRXD = _pinRx;
	NRF_UART0->PSELTXD = _pinTx;

	//NRF_UART0->CONFIG = NRF_UART0->CONFIG_HWFC_ENABLED; // Do not enable hardware flow control.
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud38400;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud57600;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud76800;
//	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud115200;
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud230400; // Highest baudrate that still worked.

	// Enable UART
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
#ifdef INIT_UART_PIN
	nrf_gpio_pin_toggle(INIT_UART_PIN);
#endif
}

void deinit_uart() {
	if (!_initializedUart) {
		return;
	}
	_initializedUart = false;
#ifdef INIT_UART_PIN
	nrf_gpio_pin_toggle(INIT_UART_PIN);
#endif

	// Disable UART
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos;
#ifdef INIT_UART_PIN
	nrf_gpio_pin_toggle(INIT_UART_PIN);
#endif
}

void init_rx() {
	if (_initializedRx) {
		return;
	}
	_initializedRx = true;
#ifdef INIT_RX_PIN
	nrf_gpio_pin_toggle(INIT_RX_PIN);
#endif

	// Enable RX ready interrupts
	NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Msk;
	// TODO: handle error event.
//	NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Msk | UART_INTENSET_ERROR_Msk | UART_INTENSET_RXTO_Msk;

	// Start RX
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
#ifdef INIT_RX_PIN
	nrf_gpio_pin_toggle(INIT_RX_PIN);
#endif
}

void deinit_rx() {
	if (!_initializedRx) {
		return;
	}
	_initializedRx = false;
#ifdef INIT_RX_PIN
	nrf_gpio_pin_toggle(INIT_RX_PIN);
#endif

	// Disable interrupt
	NRF_UART0->INTENCLR = UART_INTENSET_RXDRDY_Msk | UART_INTENSET_ERROR_Msk | UART_INTENSET_RXTO_Msk;

	// Stop RX
	NRF_UART0->TASKS_STOPTX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
	NRF_UART0->EVENTS_ERROR = 0;
	NRF_UART0->EVENTS_RXTO = 0;
#ifdef INIT_RX_PIN
	nrf_gpio_pin_toggle(INIT_RX_PIN);
#endif
}

void init_tx() {
	if (_initializedTx) {
		return;
	}
	_initializedTx = true;
#ifdef INIT_TX_PIN
	nrf_gpio_pin_toggle(INIT_TX_PIN);
#endif

	// Start TX
	NRF_UART0->TASKS_STARTTX = 1;
	NRF_UART0->EVENTS_TXDRDY = 0;
#ifdef INIT_TX_PIN
	nrf_gpio_pin_toggle(INIT_TX_PIN);
#endif
}

void deinit_tx() {
	if (!_initializedTx) {
		return;
	}
	_initializedTx = false;
#ifdef INIT_TX_PIN
	nrf_gpio_pin_toggle(INIT_TX_PIN);
#endif

	// Stop TX
	NRF_UART0->TASKS_STOPTX = 1;
	NRF_UART0->EVENTS_TXDRDY = 0;
#ifdef INIT_TX_PIN
	nrf_gpio_pin_toggle(INIT_TX_PIN);
#endif
}

void serial_init(serial_enable_t enabled) {
#if SERIAL_VERBOSITY<SERIAL_NONE
	_state = enabled;
	init();
	switch (enabled) {
	case SERIAL_ENABLE_NONE:
		deinit_rx();
		deinit_tx();
		deinit_uart();
		break;
	case SERIAL_ENABLE_RX_ONLY:
		init_uart();
		init_rx();
		deinit_tx();
		break;
	case SERIAL_ENABLE_RX_AND_TX:
		init_uart();
		init_rx();
		init_tx();
		break;
	}
#else
	// Disable UART
	deinit_rx();
	deinit_tx();
	deinit_uart();
#endif
}

void serial_enable(serial_enable_t enabled) {
	serial_init(enabled);
}

serial_enable_t serial_get_state() {
	return _state;
}

inline void _writeByte(uint8_t val) {
#if SERIAL_VERBOSITY<SERIAL_READ_ONLY
//	if (_initializedTx) { // Check this in functions that call this function.
		NRF_UART0->EVENTS_TXDRDY = 0;
		NRF_UART0->TXD = val;
		while(NRF_UART0->EVENTS_TXDRDY != 1) {}
//	}
#endif
}

/**
 * A write function with a format specifier.
 */
int write(const char *str, ...) {
#if SERIAL_VERBOSITY<SERIAL_BYTE_PROTOCOL_ONLY
	if (!_initializedTx) {
		return 0;
	}
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
		va_end(ap);
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)buffer, len);
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
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
	if (!_initializedTx) {
		return;
	}
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
	if (_initializedTx) {
		_writeByte(UART_START_BYTE);
	}
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
	if (NRF_UART0->EVENTS_ERROR && nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_ERROR)) {
#ifdef ERROR_PIN
		nrf_gpio_pin_toggle(ERROR_PIN);
#endif
//		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_ERROR);
		// TODO: disable rx and error interrupts, stop UART, call nrf_uart_errorsrc_get_and_clear().
		// Once RXTO triggers, it actually stopped.
	}

	else if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY) && nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_RXDRDY)) {
#ifdef RX_PIN
		nrf_gpio_pin_toggle(RX_PIN);
#endif
		// Clear event _before_ reading the data.
		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);

		// Read RXD only once.
		readByte = nrf_uart_rxd_get(NRF_UART0);
		UartProtocol::getInstance().onRead(readByte);
#ifdef RX_PIN
		nrf_gpio_pin_toggle(RX_PIN);
#endif
	}

	else if (NRF_UART0->EVENTS_RXTO && nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_RXTO)) {
#ifdef TIMEOUT_PIN
		nrf_gpio_pin_toggle(TIMEOUT_PIN);
#endif
//		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXTO);
		// TODO: init and start UART.
	}

}
