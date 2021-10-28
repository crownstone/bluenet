/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

typedef uint8_t* buffer_ptr_t;
typedef uint16_t cs_buffer_size_t;
typedef uint16_t cs_ret_code_t; // see protocol/cs_ErrorCodes.h
typedef uint16_t cs_control_cmd_t;
typedef uint8_t  stone_id_t; // Stone ID 0 is invalid.
typedef uint16_t device_id_t;
typedef uint16_t size16_t;
//! Boolean with fixed size.
typedef uint8_t BOOL;
typedef uint8_t cs_state_id_t;


typedef uint8_t  adc_buffer_id_t;
typedef uint8_t  adc_channel_id_t;
typedef uint8_t  adc_pin_id_t;
typedef uint16_t adc_sample_value_id_t;
typedef int16_t  adc_sample_value_t;
typedef uint8_t  adc_buffer_seq_nr_t;

// Actually wanted something like: typedef uint24_t cs_tracked_device_token_t;
#define TRACKED_DEVICE_TOKEN_SIZE 3

/**
 * Length of a MAC address
 */
static constexpr uint8_t MAC_ADDRESS_LEN = 6;

struct __attribute__((__packed__)) mac_address_t {
	uint8_t data[MAC_ADDRESS_LEN];

	void copyFrom(const void* macAddress) { memcpy(data, macAddress, MAC_ADDRESS_LEN); }

	bool operator==(const mac_address_t& other) { return memcmp(this, &other, MAC_ADDRESS_LEN) == 0; }
};

struct __attribute__((__packed__)) cs_uuid128_t{
  uint8_t uuid128[16];
};
