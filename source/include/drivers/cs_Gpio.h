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

#define TOTAL_PIN_COUNT GPIO_INDEX_COUNT + BUTTON_COUNT + LED_COUNT

typedef uint8_t pin_t;

enum GpioDirection { INPUT = 1, OUTPUT = 2, SENSE = 3 };

enum GpioPullResistor { NONE = 0, UP = 1, DOWN = 2 };

enum GpioPolarity { HITOLO = 1, LOTOHI = 2, TOGGLE = 3 };

typedef struct pin_info_t {
	bool event;
	uint8_t direction;
} pin_info_t;

class Gpio : public EventListener {

public:
	//! Get singleton instance
	static Gpio& getInstance();

	//! Initialize (from cs_Crownstone as driver)
	void init(const boards_config_t& board);

	//! Handle incoming events
	void handleEvent(event_t& event);

	//! Register event (from event handler)
	void registerEvent(pin_t pin);

private:
	//! Constructor
	Gpio();

	//! This class is a singleton, deny implementation
	Gpio(Gpio const&);

	//! This class is a singleton, deny implementation
	void operator=(Gpio const&);

	//! Get regular ticks to send events
	void tick();

	//! Array of virtual pin info
	pin_info_t _pins[TOTAL_PIN_COUNT];

	//! Return whether pin exists on board
	bool pinExists(uint8_t pin_index);

	/**
	 * Return physical pin from virtual pin index.
	 * Returns PIN_NONE when it does not exist (for the configured board).
	 */
	pin_t getPin(uint8_t pin_index);

	//! Return whether pin is a LED
	bool isLedPin(uint8_t pin_index);

	//! Configure pin
	cs_ret_code_t configure(uint8_t pin_index, GpioDirection direction, GpioPullResistor pull, GpioPolarity polarity);

	//! Write to pin
	/**
	 * Write to pin.
	 * Assumes pin has been configured as output.
	 */
	cs_ret_code_t write(uint8_t pin_index, uint8_t* buf, uint8_t& length);

	//! Read from pin
	cs_ret_code_t read(uint8_t pin_index, uint8_t* buf, uint8_t& length);

	//! Initialized flag
	bool _initialized;

	//! Board configuration
	const boards_config_t* _boardConfig;
};
