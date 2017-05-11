/*
 * Author: Bart van Vliet
 * Copyright: Crownstone B.V. (https://crownstone.rocks)
 * Date: Apr 25, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <limits>
#include "protocol/mesh/cs_MeshMessageCounter.h"

//! Size of the stick part of the lollipop (best to be an even number)
#define MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT 200

//! Size of the looping part of the lollipop
#define MESH_MESSAGE_COUNTER_LOOP_SIZE (std::numeric_limits<uint32_t>::max() - MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT + 1)

MeshMessageCounter::MeshMessageCounter(): _counter(0) {

}

uint32_t MeshMessageCounter::getVal() const {
	return _counter;
}

bool MeshMessageCounter::setVal(uint32_t val) {
	_counter = val;
	return true;
}

MeshMessageCounter& MeshMessageCounter::operator++() {
	if (_counter == std::numeric_limits<uint32_t>::max()) {
		_counter = MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT;
	}
	else {
		++_counter;
	}
	return *this;
}

int32_t MeshMessageCounter::calcDelta(uint32_t counter) {
//	int64_t oldCounter = _counter;
//	int64_t newCounter = counter;
//
//	if (oldCounter < MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT)	{
//		int64_t delta = newCounter - oldCounter;
//		return (delta > std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max() : delta;
//	}
//	else if (newCounter < MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT) {
//		int64_t delta = newCounter - oldCounter;
//		return (delta < std::numeric_limits<int32_t>::min()) ? std::numeric_limits<int32_t>::min() : delta;
//	}
//	else {
//		if (newCounter >= oldCounter) {
//			//! If the separation is larger than half of the loop size,
//			//! then the old counter is actually ahead of the new counter (but wrapped around).
//			if ((newCounter - oldCounter) > (MESH_MESSAGE_COUNTER_LOOP_SIZE / 2)) {
//				return newCounter - (oldCounter + MESH_MESSAGE_COUNTER_LOOP_SIZE); // Negative value
//			}
//			else {
//				return (newCounter - oldCounter); // Positive value
//			}
//		}
//		else {
//			//! If the separation is larger than half of the loop size,
//			//! then the new counter is actually ahead of the old counter (but wrapped around).
//			if ((oldCounter - newCounter) > (MESH_MESSAGE_COUNTER_LOOP_SIZE / 2)) {
//				return (newCounter + MESH_MESSAGE_COUNTER_LOOP_SIZE) - oldCounter; // Positive value
//			}
//			else {
//				return (newCounter - oldCounter); // Negative value
//			}
//		}
//	}
	return calcDelta(_counter, counter);
}

int32_t MeshMessageCounter::calcDelta(uint32_t oldCtr, uint32_t newCtr) {
	int64_t oldCounter = oldCtr;
	int64_t newCounter = newCtr;

	if (oldCounter < MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT)	{
		int64_t delta = newCounter - oldCounter;
		return (delta > std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max() : delta;
	}
	else if (newCounter < MESH_MESSAGE_COUNTER_LOLLIPOP_LIMIT) {
		int64_t delta = newCounter - oldCounter;
		return (delta < std::numeric_limits<int32_t>::min()) ? std::numeric_limits<int32_t>::min() : delta;
	}
	else {
		if (newCounter >= oldCounter) {
			//! If the separation is larger than half of the loop size,
			//! then the old counter is actually ahead of the new counter (but wrapped around).
			if ((newCounter - oldCounter) > (MESH_MESSAGE_COUNTER_LOOP_SIZE / 2)) {
				return newCounter - (oldCounter + MESH_MESSAGE_COUNTER_LOOP_SIZE); // Negative value
			}
			else {
				return (newCounter - oldCounter); // Positive value
			}
		}
		else {
			//! If the separation is larger than half of the loop size,
			//! then the new counter is actually ahead of the old counter (but wrapped around).
			if ((oldCounter - newCounter) > (MESH_MESSAGE_COUNTER_LOOP_SIZE / 2)) {
				return (newCounter + MESH_MESSAGE_COUNTER_LOOP_SIZE) - oldCounter; // Positive value
			}
			else {
				return (newCounter - oldCounter); // Negative value
			}
		}
	}
}

