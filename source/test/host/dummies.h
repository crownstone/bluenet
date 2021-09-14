#pragma once

typedef unsigned char ble_uuid128_t;

typedef struct ble_uuid_t {
	uint16_t uuid;
	uint8_t type;
} ble_uuid_t;

#define BLE_UUID_TYPE_UNKNOWN 0x00

typedef uint32_t ret_code_t;
