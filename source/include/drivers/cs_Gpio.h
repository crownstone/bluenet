/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 18 Feb., 2021
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <events/cs_EventListener.h>
#include <vector>

enum GpioDirection { INPUT = 1, OUTPUT = 2, SENSE = 3 };

enum GpioPullResistor { NONE = 0, UP = 1, DOWN = 2 };

enum GpioPolarity { HITOLO = 1, LOTOHI = 2, TOGGLE = 3 };

class Gpio : public EventListener {

public:

	//! Get singleton instance
	static Gpio& getInstance();

	//! Initialize (from cs_Crownstone as driver)
	void init(const boards_config_t & board);

	//! Handle incoming events
	void handleEvent(event_t & event);

private:

	//! Constructor
	Gpio();

	//! This class is a singleton, deny implementation
	Gpio(Gpio const&);

	//! This class is a singleton, deny implementation
	void operator=(Gpio const &);

	//! Map from virtual pins to physical pins
	std::vector<uint8_t> _pins;

	//! Configure pin
	void configure(uint8_t pin_index, GpioDirection direction, GpioPullResistor pull, GpioPolarity polarity);

	//! Write to pin
	void write(uint8_t pin_index, uint8_t *buf, uint8_t & length);

	//! Read from pin
	void read(uint8_t pin_index, uint8_t *buf, uint8_t & length);

	//! Initialized flag
	bool _initialized;
};
