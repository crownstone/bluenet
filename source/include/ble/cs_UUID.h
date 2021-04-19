/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <protocol/cs_Typedefs.h>

/**
 * Class that enables the use of 128 bit service UUIDs.
 *
 * There are predefined 16 bit UUIDs, which can be found here:
 * https://www.bluetooth.com/specifications/assigned-numbers/
 *
 * Vendor specific UUIDs are usually 128 bit.
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
 *
 * TODO: rename to cs_ServiceUuid
 * TODO: maybe change function names "from...." to something else and make clear you can call them multiple times.
 */
class UUID {
public:
	/**
	 * Constructor
	 */
	UUID();

	UUID(ble_uuid_t uuid);

	/**
	 * Set UUID from a 128b UUID string.
	 *
	 * The 128b UUID will be registered at the soft device.
	 *
	 * @param[in] fullUuid        In the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX.
	 */
	cs_ret_code_t fromFullUuid(const char* fullUuid);

	/**
	 * Set UUID from a 128b UUID.
	 *
	 * The 128b UUID will be registered at the soft device.
	 *
	 * @param[in] fullUuid        The 128b UUID.
	 */
	cs_ret_code_t fromFullUuid(const ble_uuid128_t& fullUuid);

	/**
	 * Set UUID from a 16b UUID.
	 *
	 * @param[in]                 The 16b UUID.
	 */
	cs_ret_code_t fromShortUuid(uint16_t shortUuid);

	/**
	 * Set UUID derived from an existing 128b base UUID.
	 *
	 * @param[in] baseUuid        The base UUID.
	 * @param[in] shortUuid       Replaces bytes 12 and 13 of the base UUID.
	 */
	cs_ret_code_t fromBaseUuid(const UUID& baseUuid, uint16_t shortUuid);

	/**
	 * Get the UUID that can be used for soft device operations.
	 */
	ble_uuid_t getUuid() const;

	/**
	 * Convenience constructors, will crash the firmware instead of returning an error code.
	 */
	UUID(const char* fullUuid);
	UUID(uint16_t shortUuid);
	UUID(const UUID& baseUuid, uint16_t shortUuid);

	bool operator==(const UUID& other);

private:
	ble_uuid_t _uuid = {
			.uuid = 0,
			.type = BLE_UUID_TYPE_UNKNOWN
	};

	// Same as the public functions, but return an NRF code.
	ret_code_t fromShortUuidInternal(uint16_t shortUuid);
	ret_code_t fromBaseUuidInternal(const UUID& baseUuid, uint16_t shortUuid);
	ret_code_t fromFullUuidInternal(const ble_uuid128_t& fullUuid);

	/**
	 * Registers the 128b UUID, without checking if already registered.
	 *
	 * @return NRF return code.
	 */
	ret_code_t add(const ble_uuid128_t& fullUuid);

	/**
	 * Checks if the 128b UUID has been registered, and sets _uuid if so.
	 *
	 * @return NRF return code.
	 */
	ret_code_t getFromCache(const ble_uuid128_t& fullUuid);

	/**
	 * Removes a 128b UUID from the soft device.
	 *
	 * Currently, can only remove the last added UUID.
	 *
	 * @return NRF return code.
	 */
	static ret_code_t rem(const ble_uuid_t& uuid);

	/**
	 * Get the CS return code from an nrf return code.
	 */
	static cs_ret_code_t fromNrfCode(ret_code_t nrfCode);
};
