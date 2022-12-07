/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <test/cs_TestAccess.h>
#include <events/cs_Event.h>
#include <logging/cs_Logger.h>

// #include <cs_Crownstone.h> // TODO

class Crownstone;

template<>
class TestAccess<Crownstone> {
public:
	static uint32_t _tickCount;

	static void tick() {
		_tickCount++;
		event_t tickEvent(CS_TYPE::EVT_TICK, &_tickCount, sizeof(_tickCount));
		tickEvent.dispatch();
	}
};

uint32_t TestAccess<Crownstone>::_tickCount = 0;
