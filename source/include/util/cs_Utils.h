/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 29, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <string>
#include "drivers/cs_Serial.h"
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


/** Macro that returns the length of an array
 *
 * @a the array whose length should be calculated
 *
 * @return the length of the array
 */
#define SIZEOF_ARRAY( a ) (int)(sizeof( a ) / sizeof( a[ 0 ] ))

template<typename T>
void printInlineArray(T* arr, uint16_t len) {
	__attribute__((unused)) uint8_t* ptr = (uint8_t*)arr;
	for (int i = 0; i < len; ++i) {
		_log(SERIAL_DEBUG, " %02X", ptr[i]);
		if ((i+1) % 32 == 0) {
			_log(SERIAL_DEBUG, SERIAL_CRLF);
		}
	}
}

template<typename T>
void printArray(T* arr, uint16_t len) {
	printInlineArray(arr, len);
	_log(SERIAL_DEBUG, SERIAL_CRLF);
}

template<typename T>
void printAddress(T* arr, uint16_t len) {
	__attribute__((unused)) uint8_t* ptr = (uint8_t*)arr;
	for (int i = len - 1; i > 0; i=i-1) {
		_log(SERIAL_DEBUG, "%02X:", ptr[i]);
	}
	_log(SERIAL_DEBUG, "%02X", ptr[0]);
	_log(SERIAL_DEBUG, SERIAL_CRLF);
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
	uint8_t *p = (uint8_t*)malloc(1);
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
	return value & 1 << bit;
}

template<typename T>
inline bool setBit(T& value, uint8_t bit) {
	return value |= 1 << bit;
}

template<typename T>
inline bool clearBit(T& value, uint8_t bit) {
	return value &= ~(1 << bit);
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
	foundData->data = NULL;
	foundData->len = 0;
	while (index < advLen-1) {
		uint8_t fieldLen = advData[index];
		uint8_t fieldType = advData[index+1];
		if (index + fieldLen >= advLen) {
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


/**
 * @brief Calculates a hash of given data.
 *
 * @param[in] Pointer to the data.
 * @param[in] Size of the data.
 * @retval    The hash.
 */
inline uint16_t calcHash(const uint8_t* data, const uint16_t size) {
	// Used implementation from here: http://www.cse.yorku.ca/~oz/hash.html
	uint16_t hash = 5381;
	for (int i=0; i<size; ++i) {
		hash = ((hash << 5) + hash) + data[i];
	}
	return hash;
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
