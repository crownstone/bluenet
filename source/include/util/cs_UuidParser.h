/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 18 Mar., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once
#include <cstdint>

#define CS_UUID_STR_LEN 36

namespace BLEutil {


/**
 * Parses a hex char to an integer.
 */
constexpr int parseHexChar(const char chr) {
	if ('0' <= chr && chr <= '9') {
		return chr - '0';
	}
	else if ('a' <= chr && chr <= 'f') {
		return chr - 'a' + 10;
	}
	else if ('A' <= chr && chr <= 'F') {
		return chr - 'A' + 10;
	}
	return 0;
}

/**
 * Parses 2 hex chars to an integer.
 */
constexpr int parseHexPair(const char *str) {
	return (parseHexChar(str[0]) << 4) | (parseHexChar(str[1]) << 0);
}

/**
 * Parse UUID string (XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX) to a byte array.
 */
constexpr bool parseUuid(const char *str, int stringSize, uint8_t* array, int arraySize) {
	if (stringSize != CS_UUID_STR_LEN || arraySize < 16) {
		return false;
	}
	//   0        8+1  12+2 16+3 20+4
	//   XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
	//   0 2 4 6  9    14   19   24
	array[15] = parseHexPair(str);
	array[14] = parseHexPair(str + 2);
	array[13] = parseHexPair(str + 4);
	array[12] = parseHexPair(str + 6);
	array[11] = parseHexPair(str + 8  + 1);
	array[10] = parseHexPair(str + 10 + 1);
	array[9]  = parseHexPair(str + 12 + 2);
	array[8]  = parseHexPair(str + 14 + 2);
	array[7]  = parseHexPair(str + 16 + 3);
	array[6]  = parseHexPair(str + 18 + 3);
	array[5]  = parseHexPair(str + 20 + 4);
	array[4]  = parseHexPair(str + 22 + 4);
	array[3]  = parseHexPair(str + 24 + 4);
	array[2]  = parseHexPair(str + 26 + 4);
	array[1]  = parseHexPair(str + 28 + 4);
	array[0]  = parseHexPair(str + 30 + 4);
	return true;
}

}
