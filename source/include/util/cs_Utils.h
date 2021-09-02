/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 29, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <string>
#include <logging/cs_Logger.h>
#include "protocol/cs_ErrorCodes.h"
#include "structs/cs_PacketsInternal.h"

/** @namespace BLEutil
 *
 * Utilities, e.g. for printing over UART.
 */
namespace BLEutil {

/** Convert a short (uint16_t) from LSB (little-endian) to
 * MSB (big-endian) and vice versa
 *
 * @val the value to be converted
 *
 * @return the converted value
 */
inline uint16_t convertEndian16(uint16_t val) {
	return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
}

/** Convert an integer (uint32_t) from LSB (little-endian) to
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

/**
 * Macro that rounds up to the next multiple of given number
 *
 * Does not work for negative values.
 */
#define CS_ROUND_UP_TO_MULTIPLE_OF(num, multiple) (((num + multiple - 1) / multiple) * multiple)
/**
 * Macro that rounds up to the next multiple of given number that is a power of 2.
 *
 * Also works for negative values.
 */
#define CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(num, multiple) ((num + multiple - 1) & -multiple)

/**
 * Get number of items in an array.
 *
 * Usage: auto count = ArraySize(myArray);
 */
template<class T, size_t N>
constexpr auto ArraySize(T(&)[N]) {
	return N;
}

template<typename T>
void printAddress(T* arr, uint16_t len, uint8_t verbosity = SERIAL_DEBUG) {
	__attribute__((unused)) uint8_t* ptr = (uint8_t*)arr;
	for (int i = len - 1; i > 0; i=i-1) {
		_log(verbosity, false, "%02X:", ptr[i]);
	}
	_log(verbosity, true, "%02X", ptr[0]);
}

template<typename T>
std::string toBinaryString(T& value) {
	std::string result("");
	for (int i = sizeof(T) * 8 - 1; i >= 0; --i) {
		result += value & (1 << i) ? "1" : "0";
	}
	return result;
}

inline void print_heap(const std::string & msg) {
	uint8_t *p = (uint8_t*)malloc(128);
	LOGd("%s %p", msg.c_str(), p);
	free(p);
}

inline void print_stack(const std::string & msg) {
	void* sp;
	asm("mov %0, sp" : "=r"(sp) : : );
	LOGd("%s %p", msg.c_str(), (uint8_t*)sp);
}

template<typename T>
inline bool isBitSet(const T value, uint8_t bit) {
	return value & (1 << bit);
}

template<typename T>
inline bool setBit(T& value, uint8_t bit) {
	return value |= (1 << bit);
}

template<typename T>
inline bool clearBit(T& value, uint8_t bit) {
	return value &= ~(1 << bit);
}

/**
 * Returns the index of the lowest bit set in given value.
 * If no bits are set, returns number of bits of value type.
 *
 * Examples for uint8_t:
 * Value | Bit representation | Lowest bit
 * 0     | 000000000          | 8
 * 1     | 000000001          | 0
 * 4     | 000000100          | 2
 * 132	 | 100000100          | 2
 */
template<typename T>
inline constexpr T lowestBitSet(T value) {
	// TODO: there is a faster implementation for this.
	T numBits = sizeof(T) * 8;
	for (T i = 0; i < numBits; i++) {
		if (value & 1) {
			return i;
		}
		value >>= 1;
	}
	return numBits;
}



/**
 * Returns true when newValue is newer than previousValue, for a value that is increased all the time and overflows.
 */
inline bool isNewer(uint8_t previousValue, uint8_t newValue) {
	int8_t diff = newValue - previousValue;
	return diff > 0;
}

/**
 * @brief Parses advertisement data, providing length and location of the field in case matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data. See https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 * @param[in]  Pointer to advertisement data.
 * @param[in]  Advertisement data length.
 * @param[out] If data type requested is found: pointer to data of given type.
 * @param[out] If data type requested is found: length of data of given type.
 *
 * @retval ERR_SUCCESS if the data type is found in the report.
 * @retval ERR_NOT_FOUND if the type could not be found.
 */
inline static cs_ret_code_t findAdvType(uint8_t type, uint8_t* advData, uint8_t advLen, cs_data_t* foundData) {
	int index = 0;
	foundData->data = nullptr;
	foundData->len = 0;
	while (index < advLen-1) {
		uint8_t fieldLen = advData[index];
		uint8_t fieldType = advData[index + 1];
		// Check if length is not 0 or larger than remaining advertisement data.
		if (fieldLen == 0 || index + 1 + fieldLen > advLen) {
			return ERR_NOT_FOUND;
		}

		if (fieldType == type) {
			foundData->data = &advData[index+2];
			foundData->len = fieldLen-1;
			return ERR_SUCCESS;
		}
		index += fieldLen+1;
	}
	return ERR_NOT_FOUND;
}



inline uint32_t getInterruptLevel() {
//	return __get_IPSR() & 0x1FF;
	return __get_IPSR();
//	0 = Thread mode
//	1 = Reserved
//	2 = NMI
//	3 = HardFault
//	4 = MemManage
//	5 = BusFault
//	6 = UsageFault
//	7-10 = Reserved
//	11 = SVCall
//	12 = Reserved for Debug
//	13 = Reserved
//	14 = PendSV
//	15 = SysTick
//	16 = IRQ0.
//	17 = IRQ1.
//	18 = IRQ2.
//	..
//	n+16 = IRQn
}

}
