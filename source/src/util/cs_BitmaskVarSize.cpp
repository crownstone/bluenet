/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 1, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <util/cs_BitmaskVarSize.h>

#include <cstdlib>
#include <cstring>

BitmaskVarSize::BitmaskVarSize() {}

BitmaskVarSize::~BitmaskVarSize() {
	if (_bitmask != nullptr) {
		free(_bitmask);
	}
}

bool BitmaskVarSize::setNumBits(uint8_t numBits) {
	if (_bitmask != nullptr) {
		free(_bitmask);
	}
	if (numBits == 0) {
		_numBits = 0;
		_bitmask = nullptr;
		return true;
	}
	_bitmask = (uint8_t*)malloc(getNumBytes(numBits));
	if (_bitmask == nullptr) {
		_numBits = 0;
		return false;
	}
	_numBits = numBits;
	clearAllBits();
	return true;
}

bool BitmaskVarSize::setBit(uint8_t bit) {
	if (bit >= _numBits) {
		return false;
	}
	_bitmask[bit / 8] |= (1 << (bit % 8));
	return true;
}

bool BitmaskVarSize::clearBit(uint8_t bit) {
	if (bit >= _numBits) {
		return false;
	}
	_bitmask[bit / 8] &= ~(1 << (bit % 8));
	return true;
}

bool BitmaskVarSize::isSet(uint8_t bit) {
	if (bit >= _numBits) {
		return false;
	}
	return _bitmask[bit / 8] & (1 << (bit % 8));
}

void BitmaskVarSize::clearAllBits() {
	if (_numBits == 0) {
		return;
	}
	uint8_t numBytes = getNumBytes(_numBits);
	memset(_bitmask, 0, numBytes);

	// Set bits that are not used.
	uint8_t numRemainingBits = _numBits % 8;
	uint8_t remainingBitmask = (1 << numRemainingBits) - 1;
	if (numRemainingBits) {
		_bitmask[numBytes - 1] = ~remainingBitmask;
	}
}

bool BitmaskVarSize::isAllBitsSet() {
	//	// First check all bytes that should have all 8 bits set.
	//	uint8_t numFullBytes = _numBits / 8;
	//	for (uint8_t i=0; i < numFullBytes; ++i) {
	//		if (_bitmask[i] != 0xFF) {
	//			return false;
	//		}
	//	}
	//
	//	// If any bits are remaining, the last byte should match a bitmask with all those bits set.
	//	uint8_t numRemainingBits = _numBits % 8;
	//	uint8_t remainingBitmask = (1 << numRemainingBits) - 1;
	//	if (numRemainingBits && (_bitmask[numFullBytes] & remainingBitmask) != remainingBitmask) {
	//		return false;
	//	}
	//	return true;

	// Since clearAllBits() sets unused bits, we can simply check if all allocated bits are set.
	for (uint8_t i = 0; i < getNumBytes(_numBits); ++i) {
		if (_bitmask[i] != 0xFF) {
			return false;
		}
	}
	return true;
}

uint8_t BitmaskVarSize::getNumBytes(uint8_t numBits) {
	return (numBits + 7) / 8;  // 8 bits per byte, rounded up.
}
