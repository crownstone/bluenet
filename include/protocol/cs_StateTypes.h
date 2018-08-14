/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 8, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventTypes.h>

/** State variable types
 * Also used as event types
 * Use in the characteristic to read and write state variables in <CommonService>.
 */
enum StateTypes {
	STATE_RESET_COUNTER = State_Base,     // 0x80 - 128
	STATE_SWITCH_STATE,                   // 0x81 - 129
	STATE_ACCUMULATED_ENERGY,             // 0x82 - 130
	STATE_POWER_USAGE,		              // 0x83 - 131
	STATE_TRACKED_DEVICES,                // 0x84 - 132
	STATE_SCHEDULE,                       // 0x85 - 133
	STATE_OPERATION_MODE,                 // 0x86 - 134
	STATE_TEMPERATURE,                    // 0x87 - 135
	STATE_TIME,                           // 0x88 - 136
	STATE_FACTORY_RESET,                  // 0x89 - 137
	STATE_LEARNED_SWITCHES,               // 0x8A - 138
	STATE_ERRORS,                         // 0x8B - 139
	STATE_ERROR_OVER_CURRENT,             // 0x8C - 140
	STATE_ERROR_OVER_CURRENT_PWM,         // 0x8D - 141
	STATE_ERROR_CHIP_TEMP,                // 0x8E - 142
	STATE_ERROR_PWM_TEMP,                 // 0x8F - 143
	STATE_IGNORE_BITMASK,                 // 0x90 - 144
	STATE_IGNORE_ALL,                     // 0x91 - 145
	STATE_IGNORE_LOCATION,                // 0x92 - 146
	STATE_ERROR_DIMMER_ON_FAILURE,        // 0x93 - 147
	STATE_ERROR_DIMMER_OFF_FAILURE,       // 0x94 - 148

	STATE_TYPES
}; // Current max is 255 (0xFF), see cs_EventTypes.h

#ifndef TYPIFY
#define TYPIFY(NAME) NAME ## _TYPE 
#endif

typedef  uint8_t TYPIFY(STATE_RESET_COUNTER);
typedef  uint8_t TYPIFY(STATE_SWITCH_STATE);
typedef  uint8_t TYPIFY(STATE_ACCUMULATED_ENERGY);
typedef  uint8_t TYPIFY(STATE_POWER_USAGE);
typedef  uint8_t TYPIFY(STATE_TRACKED_DEVICES);
typedef  uint8_t TYPIFY(STATE_SCHEDULE);
typedef  uint8_t TYPIFY(STATE_OPERATION_MODE);
typedef  uint8_t TYPIFY(STATE_TEMPERATURE);
typedef  uint8_t TYPIFY(STATE_TIME);
typedef  uint8_t TYPIFY(STATE_FACTORY_RESET);
typedef  uint8_t TYPIFY(STATE_LEARNED_SWITCHES);
typedef  uint8_t TYPIFY(STATE_ERRORS);
typedef  uint8_t TYPIFY(STATE_ERROR_OVER_CURRENT);
typedef  uint8_t TYPIFY(STATE_ERROR_OVER_CURRENT_PWM);
typedef  uint8_t TYPIFY(STATE_ERROR_CHIP_TEMP);
typedef  uint8_t TYPIFY(STATE_ERROR_PWM_TEMP);
typedef  uint8_t TYPIFY(STATE_IGNORE_BITMASK);
typedef  uint8_t TYPIFY(STATE_IGNORE_ALL);
typedef  uint8_t TYPIFY(STATE_IGNORE_LOCATION);
typedef  uint8_t TYPIFY(STATE_ERROR_DIMMER_ON_FAILURE);
typedef  uint8_t TYPIFY(STATE_ERROR_DIMMER_OFF_FAILURE);

/**
 * Custom structs
 */

struct state_vars_notifaction {
	uint8_t type;
	uint8_t* data;
	uint16_t dataLength;
};

union state_errors_t {
	struct __attribute__((packed)) {
		uint8_t overCurrent : 1;
		uint8_t overCurrentPwm : 1;
		uint8_t chipTemp : 1;
		uint8_t pwmTemp : 1;
		uint8_t dimmerOn : 1;
		uint8_t dimmerOff : 1;
		uint32_t reserved : 26;
	} errors;
	uint32_t asInt;
};

union state_ignore_bitmask_t {
	struct __attribute__((packed)) {
		uint8_t all : 1;
		uint8_t location : 1;
		uint8_t reserved : 6;
	} mask;
	uint8_t asInt;
};

