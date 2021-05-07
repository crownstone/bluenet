/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 16, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_UUID.h>
#include <cstdint>
#include <structs/cs_PacketsInternal.h>

// Commands:

struct ble_central_connect_t {
	device_address_t address;
	uint16_t timeoutMs = 3000;
};

struct ble_central_discover_t {
	const UUID* uuids;
	uint8_t uuidCount;
};

struct ble_central_read_t {
	uint16_t handle;
};

struct ble_central_write_t {
	uint16_t handle;
	cs_data_t data;
};

// Events:

struct ble_central_discovery_t {
	UUID uuid;
	uint16_t valueHandle;    // Set to BLE_GATT_HANDLE_INVALID when not existing.
	uint16_t cccdHandle;     // Set to BLE_GATT_HANDLE_INVALID when not existing.
};

struct ble_central_read_result_t {
	cs_ret_code_t retCode;
	cs_data_t data;
};

struct ble_central_notification_t {
	uint16_t handle;
	cs_const_data_t data;
};



