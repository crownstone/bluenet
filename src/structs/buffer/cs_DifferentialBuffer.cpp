/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar 16, 2015
 * License: LGPLv3+
 */
#include "common/cs_DifferentialBuffer.h"

template<>
void DifferentialBuffer<uint16_t>::serialize(uint8_t* buffer) {
	if (empty()) {
		return;
	}
	uint8_t *ptr = buffer;
	uint16_t val = _firstVal;
	*ptr++ = (val >> 8) & 0xFF;
	*ptr++ = val & 0xFF;
	for (uint16_t i=0; i<size()-1; ++i) {
		*ptr++ = _array[(_head + i)%(_capacity-1)];
	}
}

template<>
void DifferentialBuffer<uint32_t>::serialize(uint8_t* buffer) {
	if (empty()) {
		return;
	}
	uint8_t *ptr = buffer;
	uint32_t val = _firstVal;
	*ptr++ = (val >> 24) & 0xFF;
	*ptr++ = (val >> 16) & 0xFF;
	*ptr++ = (val >> 8) & 0xFF;
	*ptr++ = val & 0xFF;
	for (uint16_t i=0; i<size()-1; ++i) {
		*ptr++ = _array[(_head + i)%(_capacity-1)];
	}
}
