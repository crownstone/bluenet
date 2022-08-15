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
	Apple      = 0x004C,
	Crownstone = 0x038E,
	Tile       = 0x067C,
};

/**
 * 16 bit service UUIDs as found on the pages:
 *
 * https://www.bluetooth.com/specifications/assigned-numbers/
 * https://btprodspecificationrefs.blob.core.windows.net/assigned-values/16-bit%20UUID%20Numbers%20Document.pdf
 */
enum BleServiceUuid : uint16_t {
	TileX = 0xFEEC,  // proprietary service of Tile, inc.
	TileY = 0xFEED,  // proprietary service of Tile, inc.
	TileZ = 0xFD84,  // proprietary service of Tile, inc.
};

/**
 * Maximum length advertisement data.
 */
constexpr uint8_t ADVERTISEMENT_DATA_MAX_SIZE = 31;
