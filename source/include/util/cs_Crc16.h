/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 23, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

/**
 * Calculates or updates the CRC-16-CCITT based on given data.
 *
 * @param[in] data      Pointer to data.
 * @param[in] size      Size of data.
 * @param[in] prevCrc   Previous CRC, or null pointer for first call.
 * @return              Updated CRC of given data.
 */
uint16_t crc16(const uint8_t* data, uint16_t size, uint16_t* prevCrc = nullptr);
