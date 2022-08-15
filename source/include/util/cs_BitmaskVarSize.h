/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 1, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

/**
 * Class that holds a bitmask of variable size.
 *
 * Optimized for calling isAllBitsSet() more often than clearAllBits().
 */
class BitmaskVarSize {
public:
	BitmaskVarSize();
	~BitmaskVarSize();

	/**
	 * Set new number of bits.
	 *
	 * All bits will be cleared.
	 *
	 * Returns true on success.
	 * Returns false when failed, new number of bits will then be 0.
	 */
	bool setNumBits(uint8_t numBits);

	/**
	 * Set Nth bit.
	 */
	bool setBit(uint8_t bit);

	/**
	 * Clear Nth bit.
	 */
	bool clearBit(uint8_t bit);

	/**
	 * Check if Nth bit is set.
	 */
	bool isSet(uint8_t bit);

	/**
	 * Clear all bits.
	 */
	void clearAllBits();

	/**
	 * Check if all bits are set.
	 */
	bool isAllBitsSet();

protected:
	uint8_t _numBits  = 0;

	/**
	 * Bitmask.
	 * Looks like:
	 * byte 0: [ 7  6  5  4  3  2  1  0]
	 * byte 1: [15 14 13 12 11 10  9  8]
	 * byte 2: [23 22 21 20 19 18 17 16]
	 * So if you want 17 bits, then you need 3 bytes. So (17 / 8) rounded up.
	 * And to get bit 10, you need byte 1, shift by 2. So byte (10 / 8) rounded down, shifted by (10 % 8)
	 */
	uint8_t* _bitmask = nullptr;

	/**
	 * Get number of bytes required to store given number of bits.
	 */
	uint8_t getNumBytes(uint8_t numBits);
};
