/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 14, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

/**
 * Calculates or updates the CRC-32 based on given data.
 *
 * @param[in] data      Pointer to data.
 * @param[in] size      Size of data.
 * @param[in] prevCrc   Previous CRC, or null pointer for first call.
 * @return              Updated CRC of given data.
 */
uint32_t crc32(const uint8_t* data, uint16_t size, uint32_t* prevCrc = nullptr);
