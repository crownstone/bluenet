/*
 * Author: Alex de Mulder
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 5 Jan., 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

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
