/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 5 Jan., 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>

//! convert uint8_t to uint32_t
typedef union {
	uint8_t a[4];
	uint32_t b;
} conv8_32;

//! convert from uint8_t to uint16_t and back
typedef union {
	uint8_t a[2];
	uint16_t b;
} conv8_16;

/**
 * Random number generator using softdevice called peripheral.
 */
class RNG{
public:
	uint8_t _randomBytes[4] = {0,0,0,0};

	RNG();
	static void fillBuffer(uint8_t* buffer, uint8_t length);
	uint32_t getRandom32();
	uint16_t getRandom16();
	uint8_t  getRandom8();

};
