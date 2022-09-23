/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 18, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Serial.h>

void serial_config(uint8_t pinRx, uint8_t pinTx) {}

/**
 * Init the UART.
 * Make sure it has been configured first.
 */
void serial_init(serial_enable_t enabled) {}

/**
 * Change what is enabled.
 */
void serial_enable(serial_enable_t enabled) {}

/**
 * Set the callback
 */
void serial_set_read_callback(serial_read_callback callback) {}

/**
 * Get the state of the serial.
 */
serial_enable_t serial_get_state() {
	return SERIAL_ENABLE_RX_AND_TX;
}

/**
 * Returns whether serial is ready for TX, and has it enabled.
 */
bool serial_tx_ready() {
	return true;
}

/**
 * Write a single byte.
 */
void serial_write(uint8_t val) {}
