/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 29, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

//#include <cstdint>
#include <string>
#include <cstdlib>
//

//#include <common/cs_Types.h>

#include <drivers/cs_Serial.h>

namespace BLEutil {

/* Convert a short (uint16_t) from LSB (little-endian) to
 * MSB (big-endian) and vice versa
 *
 * @val the value to be converted
 *
 * @return the converted value
 */
inline uint16_t convertEndian16(uint16_t val) {
	return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
}

/* Convert an integer (uint32_t) from LSB (little-endian) to
 * MSB (big-endian) and vice versa
 *
 * @val the value to be converted
 *
 * @return the converted value
 */
inline uint32_t convertEndian32(uint32_t val) {
	return ((val >> 24) & 0xFF)
		 | ((val >> 8) & 0xFF00)
		 | ((val & 0xFF00) << 8)
		 | ((val & 0xFF) << 24);
}

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

inline void print_heap(const std::string & msg) {
	uint8_t *p = (uint8_t*)malloc(1);
	LOGd("%s %p", msg.c_str(), p);
	free(p);
}

inline void print_stack(const std::string & msg) {
	void* sp;
	asm("mov %0, sp" : "=r"(sp) : : );
	LOGd("%s %p", msg.c_str(), (uint8_t*)sp);
}


}
