/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <cfg/cs_Config.h>
#include <protocol/cs_ScheduleEntries.h>

/**
 * Packets (structs) that are used internally and/or over the air.
 *
 * These should be plain structs, without constructors, or member functions.
 * Instead of a constructor, you can add a function that fills a struct.
 *
 * If the definition becomes large, move it to its own file and include it in this file.
 */

/**
 * State errors: collection of errors that influence the switch behaviour.
 */
union __attribute__((__packed__)) state_errors_t {
	struct __attribute__((packed)) {
		bool overCurrent: 1;
		bool overCurrentDimmer: 1;
		bool chipTemp: 1;
		bool dimmerTemp: 1;
		bool dimmerOn: 1;
		bool dimmerOff: 1;
	} errors;
	uint32_t asInt;
};
/**
 * Sets the state errors to the default.
 */
constexpr void cs_state_errors_set_default(state_errors_t *state) {
	state->asInt = STATE_ERRORS_DEFAULT;
}

/**
 * Switch state: combination of relay and dimmer state.
 * Relay:  0 = off, 1 = on
 * Dimmer: 0 = off, 100 = fully on
 */
union __attribute__((__packed__)) __attribute__((__aligned__(4))) switch_state_t {
	struct __attribute__((packed)) {
		uint8_t dimmer : 7;
		uint8_t relay  : 1;
	} state;
	uint8_t asInt;
};
/**
 * Sets the switch state to the default.
 */
constexpr void cs_switch_state_set_default(switch_state_t *state) {
	state->asInt = STATE_SWITCH_STATE_DEFAULT;
}


#define SESSION_NONCE_LENGTH 5
struct __attribute__((packed)) session_nonce_t {
	uint8_t data[SESSION_NONCE_LENGTH];
};

// TODO: check if this length is similar to BLE_GAP_ADDR_LEN
#define MAC_ADDRESS_LEN 6
#define ADVERTISEMENT_DATA_MAX_SIZE 31

struct __attribute__((packed)) scanned_device_t {
	int8_t rssi;
	uint8_t address[MAC_ADDRESS_LEN];
	uint8_t channel;
	uint8_t dataSize;
//	uint8_t data[ADVERTISEMENT_DATA_MAX_SIZE];
	uint8_t *data;
	// See ble_gap_evt_adv_report_t
	// More possibilities: addressType, connectable, isScanResponse, directed, scannable, extended advertisements, etc.
};
