/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 22, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

#define SERVICE_DATA_VALIDATION 0xFA

enum ServiceDataFlagBits {
	SERVICE_DATA_FLAGS_DIMMING_AVAILABLE   = 0,
	SERVICE_DATA_FLAGS_MARKED_DIMMABLE     = 1,
	SERVICE_DATA_FLAGS_ERROR               = 2,
	SERVICE_DATA_FLAGS_SWITCH_LOCKED       = 3,
	SERVICE_DATA_FLAGS_TIME_SET            = 4,
	SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED = 5,
	SERVICE_DATA_FLAGS_RESERVED6           = 6,
	SERVICE_DATA_FLAGS_RESERVED7           = 7
};

enum ServiceDataUnencryptedType {
	SERVICE_DATA_TYPE_ENCRYPTED = 5,
	SERVICE_DATA_TYPE_SETUP = 6,
};

enum ServiceDataEncryptedType {
	SERVICE_DATA_TYPE_STATE = 0,
	SERVICE_DATA_TYPE_ERROR = 1,
	SERVICE_DATA_TYPE_EXT_STATE = 2,
	SERVICE_DATA_TYPE_EXT_ERROR = 3,
};

struct __attribute__((packed)) service_data_encrypted_state_t {
	uint8_t  id;
	uint8_t  switchState;
	uint8_t  flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	int32_t  energyUsed;
	uint16_t partialTimestamp;
	uint8_t  reserved;
	uint8_t  validation;
};

struct __attribute__((packed)) service_data_encrypted_error_t {
	uint8_t  id;
	uint32_t errors;
	uint32_t timestamp;
	uint8_t  flags;
	int8_t   temperature;
	uint16_t partialTimestamp;
	int16_t  powerUsageReal;
};

struct __attribute__((packed)) service_data_encrypted_ext_state_t {
	uint8_t  id;
	uint8_t  switchState;
	uint8_t  flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	int32_t  energyUsed;
	uint16_t partialTimestamp;
//	uint8_t  reserved[2];
//	uint16_t validation;
	int8_t   rssi;
	uint8_t  validation;
};

struct __attribute__((packed)) service_data_encrypted_ext_error_t {
	uint8_t  id;
	uint32_t errors;
	uint32_t timestamp;
	uint8_t  flags;
	int8_t   temperature;
	uint16_t partialTimestamp;
//	uint8_t  reserved[2];
//	uint16_t validation;
	int8_t   rssi;
	uint8_t  validation;
};

struct __attribute__((packed)) service_data_encrypted_t {
	uint8_t type;
	union {
		service_data_encrypted_state_t state;
		service_data_encrypted_error_t error;
		service_data_encrypted_ext_state_t extState;
		service_data_encrypted_ext_error_t extError;
	};
};



struct __attribute__((packed)) service_data_setup_state_t {
	uint8_t  switchState;
	uint8_t  flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	uint32_t errors;
	uint8_t  counter;
	uint8_t  reserved[4];
};

struct __attribute__((packed)) service_data_setup_t {
	uint8_t type;
	union {
		service_data_setup_state_t state;
	};
};

//! Service data struct, this data type is what ends up in the advertisement.
union service_data_t {
	struct __attribute__((packed)) {
		uint8_t  protocolVersion;
		uint8_t  deviceType;
		union {
			service_data_encrypted_t encrypted;
			service_data_setup_t setup;
			uint8_t encryptedArray[sizeof(service_data_encrypted_t)];
		};
	} params;
	uint8_t array[sizeof(params)] = {};
};
