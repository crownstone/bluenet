/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 5 Jan., 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>

/**
 * Random number generator using softdevice called peripheral.
 */
class RNG {
public:
	// Use static variant of singleton, no dynamic memory allocation
	static RNG& getInstance() {
		static RNG instance;
		return instance;
	}

	static void fillBuffer(uint8_t* buffer, uint8_t length);

	uint32_t getRandom32();
	uint16_t getRandom16();
	uint8_t getRandom8();

private:
	uint8_t _randomBytes[4];

	// Constructor
	RNG();

	// This class is singleton, deny implementation
	RNG(RNG const&);

	// This class is singleton, deny implementation
	void operator=(RNG const&);
};
