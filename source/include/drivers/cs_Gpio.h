/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 18 Feb., 2021
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>
#include <events/cs_EventListener.h>
#include <vector>

enum class GpioDirection { INPUT, OUTPUT, SENSE };

enum class GpioPullResistor { NONE, UP, DOWN };

enum class GpioPolarity { HITOLO, LOTOHI, TOGGLE };

class Gpio : public EventListener {

public:

	//! Get singleton instance
	static Gpio& getInstance();

	//! Handle incoming events
	void handleEvent(event_t & event);

private:

	//! Constructor
	Gpio();

	//! This class is a singleton, deny implementation
	Gpio(Gpio const&);

	//! This class is a singleton, deny implementation
	void operator=(Gpio const &);

	std::vector _pins;
}
