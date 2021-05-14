/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 14, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h> // For the crc32.h include
#include <util/cs_Crc32.h>

// We use the nrf implementation, as that seems a faster implementation..
uint32_t crc32(const uint8_t* data, uint16_t size, uint32_t* prevCrc) {
	return crc32_compute(data, size, prevCrc);
}
