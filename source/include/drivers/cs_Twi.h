/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Boards.h>
#include <events/cs_EventListener.h>

enum class TwiIsrEvent { Read, Write, Error };
/**
 * Class that implements twi/i2c. This on the moment only implements twi as master.
 */
class Twi : public EventListener {
public:
	/**
	 * Construct twi/i2c instance.
	 */
	static Twi& getInstance();

	/**
	 * Init twi with board configuration (nothing is happening to the pins yet).
	 *
	 * @param[in]     board        Hardware board.
	 */
	void init(const boards_config_t& board);

	/**
	 * Init twi as master on the i2c bus.
	 *
	 * @param[in]     twi          Configuration (frequency, etc.)
	 */
	void initBus(cs_twi_init_t& twi);

	/**
	 * Write data to given address.
	 *
	 * @param[in]     address      The address of the device to write to.
	 * @param[in]     data         Pointer to data to be written.
	 * @param[in]     length       The number of items to be written.
	 * @param[in]     stop         Release the bus after a write (for e.g. another i2c master).
	 */
	void write(uint8_t address, uint8_t* data, size_t length, bool stop);

	/**
	 * Read data from given address.
	 *
	 * @param[in]     address      The address of the device to read from.
	 * @param[out]    data         Pointer to data array where result will be received.
	 * @param[in,out] length       Number of items to read, and returns number of items actually read.
	 */
	void read(uint8_t address, uint8_t* data, size_t& length);

	/**
	 * Incoming events.
	 *
	 * @param[in]     event        Event from other modules in bluenet.
	 */
	void handleEvent(event_t& event);

	/**
	 * Events from the hardware.
	 */
	void isrEvent(TwiIsrEvent event);

protected:
	// Local struct to store configuration.
	static const nrfx_twi_t _twi;

private:
	Twi();
	Twi(Twi const&);
	void operator=(Twi const&);

	// Local config for driver
	nrfx_twi_config_t _config;

	// Initialized flag
	bool _initialized;

	// Initialized flag
	bool _initializedBus;

	// Event is read
	bool _eventRead;

	// Error event
	bool _eventError;
};
