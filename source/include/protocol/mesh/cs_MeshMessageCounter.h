/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 25, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

//#include <cstdint>
#include <stdint.h>

class MeshMessageCounter {
public:
	MeshMessageCounter();

	uint32_t getVal() const;

	bool setVal(uint32_t val);

	//! Pre-increment operator
	MeshMessageCounter& operator++();

	int32_t calcDelta(uint32_t newCounter);

	//! Returns the difference between two counters, think of it as: newCounter - oldCounter
	//! If larger than 0, the newCounter is newer than the oldCounter
	static int32_t calcDelta(uint32_t oldCounter, uint32_t newCounter);

private:
	uint32_t _counter;

};
