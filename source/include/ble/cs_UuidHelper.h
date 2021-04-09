/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <protocol/cs_Typedefs.h>
#include <util/cs_UuidParser.h>

/**
 * Class that enables the use of 128 bit UUIDs.
 *
 * 128b UUID strings are reverse ordered compared to the byte array, in the form:
 * "15 14 13 12 - 11 10 - 09 08 - 07 06 - 05 04 03 02 01 00"
 * but then without spaces.
 *
 * The Softdevice functions only deal with 16b UUIDS, so they introduced 1 extra byte (the 'type' field) to indicate the base UUID.
 * This base UUID has to be registered first, but can then be used with different 16b fields.
 * A base UUID would look like XXXX0000-XXXX-XXXX-XXXX-XXXXXXXXXXXX.
 * The 0000 part is then replaced by the 16b UUID.
 *
 * A typical example is a BLE service with multiple characteristics.
 * All of them will have the same base UUID, but different 16b UUIDs.
 */
class UuidHelper {
public:
	/**
	 * Get a 128b UUID from a string representation.
	 *
	 * @param[in] fullUuid        In the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX.
	 * @param[in] stringLength    Length of the fullUuid string.
	 * @return                    128b UUID, even if the string is invalid.
	 */
	static constexpr ble_uuid128_t fromString(const char* fullUuid, uint8_t len) {
		ble_uuid128_t uuid = {};
		BLEutil::parseUuid(fullUuid, len, uuid.uuid128, sizeof(uuid.uuid128));
		return uuid;
	}

	/**
	 * Get a UUID from a 16b UUID.
	 */
	static constexpr ble_uuid_t fromShortUuid(uint16_t shortUuid) {
		return ble_uuid_t {
				.type = BLE_UUID_TYPE_BLE,
				.uuid = shortUuid
		};
	}

	static ble_uuid_t fromBaseUuid(const ble_uuid_t& baseUuid, uint16_t shortUuid, cs_ret_code_t* retCode);

	/**
	 * Add a 128b UUID.
	 *
	 * @param[in] fullUuid        The 128b UUID to be registered.
	 * @param[out] resultUuid     UUID that can be used in other functions.
	 */
	static cs_ret_code_t add(const ble_uuid128_t& fullUuid, ble_uuid_t& resultUuid);

	/**
	 * Add a 128b UUID.
	 *
	 * @param[in] fullUuid        The 128b UUID to be registered.
	 * @param[out] retCode        Pointer to return code, which will be set.
	 *                            If pointer is a null pointer, any error will result in a crash.
	 * @return                    Resulting UUID.
	 */
	static ble_uuid_t add(const ble_uuid128_t& fullUuid, cs_ret_code_t* retCode);



	/**
	 * Add a 128b UUID derived from an already added 128b base UUID.
	 *
	 * @param[in] baseUuid        The base UUID that was added earlier.
	 * @param[in] shortUuid       Replaces bytes 12 and 13 of the base UUID.
	 * @param[out] resultUuid     UUID that can be used in other functions.
	 */
	static cs_ret_code_t add(const ble_uuid_t& baseUuid, uint16_t shortUuid, ble_uuid_t& resultUuid);

	/**
	 * Remove a 128b UUID.
	 *
	 * Currently, can only remove the last added UUID.
	 *
	 * @param[in] shortUuid       The short UUID.
	 */
	static cs_ret_code_t rem(const ble_uuid_t& uuid);

private:
	/**
	 * Registers the 128b UUID, without checking if already registered.
	 */
	static cs_ret_code_t add(const ble_uuid128_t& fullUuid, uint8_t& type);

	/**
	 * Checks if the 128b UUID has been registered, and sets short UUID if so.
	 */
	static cs_ret_code_t getFromCache(const ble_uuid128_t& fullUuid, ble_uuid_t& resultUuid);
};


