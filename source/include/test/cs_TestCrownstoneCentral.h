/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 20, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>

/**
 * Class to test the BleCentral class.
 *
 * Currently has hard coded values, so it's really plug and play.
 * It also serves as an example usage.
 */
class TestCrownstoneCentral: EventListener {
public:
	TestCrownstoneCentral();
	void init();

	void connect();
	void read();
	void writeGetMac();
	void writeSetup();
	void writeGetPowerSamples();
	void disconnect();

	void handleEvent(event_t & event);
private:
	uint8_t _writeStep = 0;
};

