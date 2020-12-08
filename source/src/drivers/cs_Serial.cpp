/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <app_util.h>
#include <app_util_platform.h>
#include <ble/cs_Nordic.h>
#include <cfg/cs_Boards.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>
#include <nrf.h>
#include <uart/cs_UartHandler.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

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

#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY
static char _serialBuffer[128];
#endif

void serial_config(uint8_t pinRx, uint8_t pinTx) {
	_pinRx = pinRx;
	_pinTx = pinTx;
}

// Initializes anything but the UART peripheral.
// Only to be called once.
void init() {
#if CS_SERIAL_NRF_LOG_ENABLED == 2
	return;
#endif
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
#if CS_SERIAL_NRF_LOG_ENABLED == 2
	return;
#endif
#if SERIAL_VERBOSITY > SERIAL_NONE
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

bool serial_tx_ready() {
	return _initializedTx;
}

inline void _writeByte(uint8_t val) {
#if SERIAL_VERBOSITY > SERIAL_READ_ONLY
//	if (_initializedTx) { // Check this in functions that call this function.
		NRF_UART0->EVENTS_TXDRDY = 0;
		NRF_UART0->TXD = val;
		while(NRF_UART0->EVENTS_TXDRDY != 1) {}
//	}
#endif
}


template<>
void cs_add_arg_size(size_t& size, uint8_t& numArgs, char* str) {
	size += sizeof(uart_msg_log_arg_header_t) + strlen(str);
	++numArgs;
}

template<>
void cs_add_arg_size(size_t& size, uint8_t& numArgs, const char* str) {
	size += sizeof(uart_msg_log_arg_header_t) + strlen(str);
	++numArgs;
}

template<>
void cs_write_arg(char* str) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(str);
	cs_write_arg(valPtr, strlen(str));
}

template<>
void cs_write_arg(const char* str) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(str);
	cs_write_arg(valPtr, strlen(str));
}

void cs_write_start(size_t msgSize, uart_msg_log_header_t &header) {
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_LOG, msgSize);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, reinterpret_cast<uint8_t*>(&header), sizeof(header));
}

void cs_write_arg(const uint8_t* const valPtr, size_t valSize) {
	uart_msg_log_arg_header_t argHeader;
	argHeader.argSize = valSize;
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, reinterpret_cast<uint8_t*>(&argHeader), sizeof(argHeader));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, valPtr, valSize);
}

void cs_write_end() {
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_LOG);
}

void cs_write_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint8_t* const ptr, size_t size, ElementType elementType, size_t elementSize) {
	uart_msg_log_array_header_t header;
	header.header.fileNameHash = fileNameHash;
	header.header.lineNumber = lineNumber;
	header.header.logLevel = logLevel;
	header.header.flags.newLine = addNewLine;
	header.elementType = elementType;
	header.elementSize = elementSize;
	uint16_t msgSize = sizeof(header) + size;
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_LOG_ARRAY, msgSize);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG_ARRAY, reinterpret_cast<uint8_t*>(&header), sizeof(header));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG_ARRAY, ptr, size);
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_LOG_ARRAY);
}

/**
 * A write function with a format specifier.
 */
int cs_write(const char *str, ...) {
#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY
	if (!_initializedTx) {
		return 0;
	}
	va_list ap;
	va_start(ap, str);
	int16_t len = vsnprintf(NULL, 0, str, ap);
	va_end(ap);

	if (len < 0) return len;

	// if strings are small we do not need to allocate by malloc
	if (sizeof(_serialBuffer) >= len + 1UL) {
		va_start(ap, str);
		len = vsprintf(_serialBuffer, str, ap);
		va_end(ap);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)_serialBuffer, len);
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)p_buf, len);
		free(p_buf);
	}
	return len;
#endif
	return 0;
}

void writeByte(uint8_t val) {
#if SERIAL_VERBOSITY > SERIAL_READ_ONLY
	// TODO: that's an if for every byte we write.
	if (!_initializedTx) {
		return;
	}
	_writeByte(val);
#endif
}



#if CS_SERIAL_NRF_LOG_ENABLED != 2
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
		UartHandler::getInstance().onRead(readByte);
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
#endif
