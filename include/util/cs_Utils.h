/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 29, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <cstdint>

#include <drivers/cs_Serial.h>

namespace BLEutil {

/* Convert a short (uint16_t) from LSB (little-endian) to
 * MSB (big-endian) and vice versa
 *
 * @val the value to be converted
 *
 * @return the converted value
 */
uint16_t convertEndian16(uint16_t val);

/* Convert an integer (uint32_t) from LSB (little-endian) to
 * MSB (big-endian) and vice versa
 *
 * @val the value to be converted
 *
 * @return the converted value
 */
uint32_t convertEndian32(uint32_t val);

/* Macro that returns the length of an array
 *
 * @a the array whose length should be calculated
 *
 * @return the length of the array
 */
#define SIZEOF_ARRAY( a ) (int)(sizeof( a ) / sizeof( a[ 0 ] ))

template<typename T>
void printArray(T* arr, uint16_t len) {
	for (int i = 0; i < len; ++i) {
		_log(DEBUG, " %02X", arr[i]);
	}
	_log(DEBUG, "\r\n");
}

}
