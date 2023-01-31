/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

/**
 * Driver class for serial / UART.
 *
 * Note: this class is also used by C code.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <protocol/cs_SerialTypes.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * General configuration of the serial connection. This sets the pin to be used for UART, the baudrate, the parity
 * bits, etc.
 * Should only be called once.
 */
void serial_config(uint8_t pinRx, uint8_t pinTx);

/**
 * Init the UART.
 * Make sure it has been configured first.
 */
void serial_init(serial_enable_t enabled);

/**
 * Change what is enabled.
 */
void serial_enable(serial_enable_t enabled);

/**
 * Set the callback
 */
void serial_set_read_callback(serial_read_callback callback);

/**
 * Get the state of the serial.
 */
serial_enable_t serial_get_state();

/**
 * Returns whether serial is ready for TX, and has it enabled.
 */
bool serial_tx_ready();

/**
 * Write a single byte.
 */
void serial_write(uint8_t val);

#ifdef __cplusplus
}
#endif
