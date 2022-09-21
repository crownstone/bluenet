/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 16, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_UUID.h>
#include <structs/cs_PacketsInternal.h>

#include <cstdint>

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

/**
 * A service or characteristic has been discovered.
 */
struct ble_central_discovery_t {
	//! The service UUID.
	UUID serviceUuid;

	//! The characteristic UUID, or the service UUID for a service discovery.
	UUID uuid;

	//! The value handle. 0 when it does not exist.
	uint16_t valueHandle;

	//! The CCCD handle. 0 when it does not exist.
	uint16_t cccdHandle;

	struct {
		bool broadcast : 1;
		bool read : 1;
		bool write_no_ack : 1;
		bool write_with_ack : 1;
		bool notify : 1;
		bool indicate : 1;
		bool write_signed : 1;
	} flags;
};

struct ble_central_write_result_t {
	cs_ret_code_t retCode;
	uint16_t handle;
};

struct ble_central_read_result_t {
	cs_ret_code_t retCode;
	uint16_t handle;
	cs_data_t data;
};

struct ble_central_notification_t {
	uint16_t handle;
	cs_const_data_t data;
};
