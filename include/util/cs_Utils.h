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

#if __clang__
#define STRINGIFY(str) #str
#else
#define STRINGIFY(str) str
#endif

/** @brief Variable length data encapsulation in terms of length and pointer to data */
typedef struct
{
    uint8_t     * p_data;                                         /** < Pointer to data. */
    uint16_t      data_len;                                       /** < Length of data. */
} data_t;

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
		if ((i+1) % 30 == 0) {
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
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
inline uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->data_len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type = p_data[index+1];

        if (field_type == type)
        {
            p_typedata->p_data = &p_data[index+2];
            p_typedata->data_len = field_length-1;
            return NRF_SUCCESS;
        }
        index += field_length+1;
    }
    return NRF_ERROR_NOT_FOUND;
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

}
