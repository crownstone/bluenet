/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_Serial.h>

static uint8_t _pinRx                     = 0;
static uint8_t _pinTx                     = 0;
static bool _initialized                  = false;
static bool _initializedUart              = false;
static bool _initializedRx                = false;
static bool _initializedTx                = false;
static serial_enable_t _state             = SERIAL_ENABLE_NONE;
static serial_read_callback _readCallback = NULL;

/*
 * Set the RX and TX pin. Within the bluenet firmware TX means transmission from the hardware towards e.g. a laptop.
 */
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

	// Enable interrupt handling
	uint32_t err_code;
	err_code = sd_nvic_SetPriority(UARTE0_UART0_IRQn, APP_IRQ_PRIORITY_MID);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(UARTE0_UART0_IRQn);
	APP_ERROR_CHECK(err_code);
}

void serial_set_read_callback(serial_read_callback callback) {
	_readCallback = callback;
}

/*
 * Initializes the UART peripheral.
 *
 * Hardware flow control is NOT enabled (the default). Hence, there's no call like:
 *   NRF_UART0->CONFIG = NRF_UART0->CONFIG_HWFC_ENABLED
 *
 * The baudrate is set to 230400 baud. This is the highest baudrate that works reliably.
 * Lower rates are for example 38400, 57600, and 115200 baudrates.
 *
 * TODO: Add check: serial_config() might not have been called before (_pinRx = 0, _pinTx = 0).
 */
void init_uart() {
	if (_initializedUart) {
		return;
	}
	_initializedUart   = true;

	NRF_UART0->PSELRXD = _pinRx;
	NRF_UART0->PSELTXD = _pinTx;
#if !defined(UART_BAUDRATE)
#error "UART_BAUDRATE not defined"
#endif

#if UART_BAUDRATE == 57600
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud57600;
#elif UART_BAUDRATE == 115200
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud115200;
#elif UART_BAUDRATE == 230400
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud230400;
#elif UART_BAUDRATE == 460800
	NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud460800;
#else
#error "Unknown UART_BAUDRATE"
#endif
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
}

void deinit_uart() {
	if (!_initializedUart) {
		return;
	}
	_initializedUart  = false;

	// Disable UART
	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos;
}

/*
 * Initializes incoming UART.
 *
 * TODO: handle error event and in that case the following interrupts can be set:
 * NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Msk | UART_INTENSET_ERROR_Msk | UART_INTENSET_RXTO_Msk;
 */
void init_rx() {
	if (_initializedRx) {
		return;
	}
	_initializedRx           = true;

	// Enable RX ready interrupts
	NRF_UART0->INTENSET      = UART_INTENSET_RXDRDY_Msk;

	// Start RX
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
}

void deinit_rx() {
	if (!_initializedRx) {
		return;
	}
	_initializedRx           = false;

	// Disable interrupt
	NRF_UART0->INTENCLR      = UART_INTENSET_RXDRDY_Msk | UART_INTENSET_ERROR_Msk | UART_INTENSET_RXTO_Msk;

	// Stop RX
	NRF_UART0->TASKS_STOPTX  = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
	NRF_UART0->EVENTS_ERROR  = 0;
	NRF_UART0->EVENTS_RXTO   = 0;
}

void init_tx() {
	if (_initializedTx) {
		return;
	}
	_initializedTx           = true;

	// Start TX
	NRF_UART0->TASKS_STARTTX = 1;
	NRF_UART0->EVENTS_TXDRDY = 0;
}

void deinit_tx() {
	if (!_initializedTx) {
		return;
	}
	_initializedTx           = false;

	// Stop TX
	NRF_UART0->TASKS_STOPTX  = 1;
	NRF_UART0->EVENTS_TXDRDY = 0;
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

/*
 * This function does not check _initializedTx. That's supposed to have been done by functions that call this function.
 */
inline void _serial_write(uint8_t val) {
#if SERIAL_VERBOSITY > SERIAL_READ_ONLY
	NRF_UART0->EVENTS_TXDRDY = 0;
	NRF_UART0->TXD           = val;
	while (NRF_UART0->EVENTS_TXDRDY != 1) {
	}
#endif
}

void serial_write(uint8_t val) {
#if SERIAL_VERBOSITY > SERIAL_READ_ONLY
	if (!_initializedTx) {
		return;
	}
	_serial_write(val);
#endif
}

#if CS_SERIAL_NRF_LOG_ENABLED != 2
static uint8_t readByte;

/*
 * UART interrupt handler
 *
 * TODO: Disable rx and error interrupts, stop UART, call nrf_uart_errorsrc_get_and_clear(). Once RXTO triggers, it
 * actually has stopped.
 *
 */
extern "C" void UART0_IRQHandler(void) {
	if (NRF_UART0->EVENTS_ERROR && nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_ERROR)) {
		// See above
	}
	else if (
			nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY)
			&& nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_RXDRDY)) {
		// Clear event _before_ reading the data.
		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);

		// Read RXD only once.
		readByte = nrf_uart_rxd_get(NRF_UART0);
		if (_readCallback != NULL) {
			_readCallback(readByte);
		}
	}
	else if (NRF_UART0->EVENTS_RXTO && nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_RXTO)) {
		// TODO: clear events, init and start UART.
	}
}
#endif
