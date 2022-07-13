/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 21, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Struct with all the Crownstone fields in UICR.
 * This is also used by the bootloader, so should be C compatible.
 */
typedef struct __attribute__((packed)) {
	uint32_t board;

	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			uint8_t productType;
			uint8_t region;
			uint8_t productFamily;
			uint8_t reserved;
		} fields;
		uint32_t asInt;
	} productRegionFamily;

	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			uint8_t patch;
			uint8_t minor;
			uint8_t major;
			uint8_t reserved;
		} fields;
		uint32_t asInt;
	} majorMinorPatch;

	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			uint8_t housing;
			uint8_t week; // week number
			uint8_t year; // last 2 digits of the year
			uint8_t reserved;
		} fields;
		uint32_t asInt;
	} productionDateHousing;
} cs_uicr_data_t;

#ifdef __cplusplus
}
#endif
