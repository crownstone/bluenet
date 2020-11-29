/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once
#include <cstdint>

/**
 * Bluetooth company ids of devices that Crownstones integrate with or recognize.
 *
 * Naming convention:
 * - Use the company name as mentioned on the bluetooth SIG website, and
 * - strip off any suffixes such as inc., gov., bv. and the like.
 * - Capitalization as codebase standard for enum values.
 *
 * See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers
 */
enum BleCompanyId : uint16_t {
	Apple = 0x004C,
	Crownstone = 0x038E,
	Tile = 0x067C,
};


/**
 * Maximum length advertisement data.
 */
constexpr uint8_t ADVERTISEMENT_DATA_MAX_SIZE = 31;
