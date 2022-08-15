/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>  // For the crc16.h include
#include <util/cs_Crc16.h>

// We use the nrf implementation, as that seems a faster implementation..
uint16_t crc16(const uint8_t* data, uint16_t size, uint16_t* prevCrc) {
	return crc16_compute(data, size, prevCrc);
}
