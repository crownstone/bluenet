/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 13, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Returns the index of the lowest bit set in v. (I.e. the number of leading zeroes.)
 * E.g.
 * 		1 -> 0
 * 		2 -> 1
 * 		4 -> 2
 * 		5 -> 1
 * 		0b 0001 0001 0100 0000 -> 6
 *
 * 		uint8_t  u = 0; lowestBitSet(u) == 8;
 * 		uint16_t u = 0; lowestBitSet(u) == 16;
 * 		uint32_t u = 0; lowestBitSet(u) == 32;
 *
 */
constexpr auto lowestBitSet(auto v) -> decltype(v) {
	auto numBits = sizeof(v) * 8;

	for (auto i{0}; i < numBits; i++) {
		if(v & 1) {
			return i;
		}
		v >>= 1;
	}
	return numBits;
}


