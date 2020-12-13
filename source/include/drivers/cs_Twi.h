/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <events/cs_EventListener.h>

/**
 * Class that implements twi/i2c.
 */
class Twi: public EventListener {
public:
	Twi();

	/**
	 * Init twi.
	 */
	void init(uint8_t pin_scl, uint8_t pin_sda, uint8_t address);

	/**
	 * Write data.
	 */
	void write(uint8_t *data, size_t length);

	/**
	 * Read data.
	 */
	void read(uint8_t *data, size_t length);

	/**
	 * Read on ticks and update.
	 */
	void tick();

	/**
	 * Incoming events.
	 */
	void handleEvent(event_t & event);
protected:
	/**
	 * Local struct to store configuration.
	 */
	static const nrfx_twi_t _twi;

private:
	nrfx_twi_config_t _config;

	uint8_t _address;

	// Internal buffer for reading
	uint8_t *_buf;

	// Size of internal buffer
	uint8_t _bufSize;
};
