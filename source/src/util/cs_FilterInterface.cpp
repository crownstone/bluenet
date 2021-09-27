/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


asset_id_t FilterInterface::assetId(const void* key, size_t keyLengthInBytes) {
	auto crc = crc32(static_cast<const uint8_t*>(key), keyLengthInBytes, nullptr);

	// little endian byte by byte copy.
	asset_id_t id{};
	memcpy(id.data, reinterpret_cast<uint8_t*>(&crc), 3);

	return id;
}
