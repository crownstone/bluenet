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



constexpr int parseHexChar(const char chr) {
	if ('0' <= chr && chr <= '9') {
		return chr - '0';
	}
	else if ('a' <= chr && chr <= 'f') {
		return chr - 'a';
	}
	else if ('A' <= chr && chr <= 'F') {
		return chr - 'A';
	}
	return 0;
}

constexpr int parseHex(const char *str) {
	return parseHexChar(str[0]) | (parseHexChar(str[1]) << 8);
}

/**
 * Parse UUID string (XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX) to a byte array.
 */
constexpr bool parseUuid(const char *str, int size, uint8_t* array) {
	if (size != CS_UUID_STR_LEN) {
		return false;
	}
	//   0        8+1  12+2 16+3 20+4
	//   XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
	//   0 2 4 6  9    14   19   24
	array[0]  = parseHex(str);
	array[1]  = parseHex(str + 2);
	array[2]  = parseHex(str + 4);
	array[3]  = parseHex(str + 6);
	array[4]  = parseHex(str + 8  + 1);
	array[5]  = parseHex(str + 10 + 1);
	array[6]  = parseHex(str + 12 + 2);
	array[7]  = parseHex(str + 14 + 2);
	array[8]  = parseHex(str + 16 + 3);
	array[9]  = parseHex(str + 18 + 3);
	array[10] = parseHex(str + 20 + 4);
	array[11] = parseHex(str + 22 + 4);
	array[12] = parseHex(str + 24 + 4);
	array[13] = parseHex(str + 26 + 4);
	array[14] = parseHex(str + 28 + 4);
	array[15] = parseHex(str + 30 + 4);
	return true;
}

}
