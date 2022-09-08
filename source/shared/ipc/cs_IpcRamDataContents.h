/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 31, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// The size of each slot for interprocess communication
#define BLUENET_IPC_RAM_DATA_ITEM_SIZE 24

// Update when struct bluenet_ipc_bluenet_data_t changes
#define BLUENET_IPC_BLUENET_REBOOT_DATA_MAJOR 1
#define BLUENET_IPC_BLUENET_REBOOT_DATA_MINOR 0

/**
 * The protocol struct with which the bootloader communicates with the bluenet binary through RAM. The protocol can
 * be bumped when there are changes. The dfu_version is an always increasing number that gets incremented for each
 * new bootloader change. It is the Bootloader Version number that is written in the bootloader settings file through
 * `make build_bootloader_settings`. The major, minor, and patch is e.g. 2.1.0 and is communicated over BLE and
 * used externally to check for compatibility and firmware update processes. There is another prerelease number which
 * can be used as alpha, beta, or rc. Convention for now is something like 2.1.0-rc2. The build type is an integer
 * that represented the CMAKE_BUILD_TYPE, such as Release or Debug.
 */
typedef struct {
	// Major version of the data in this struct
	uint8_t ipcDataMajor;
	// Minor version of the data in this struct
	uint8_t ipcDataMinor;
	uint16_t dfuVersion;
	uint8_t bootloaderMajor;
	uint8_t bootloaderMinor;
	uint8_t bootloaderPatch;
	// A prerelease value. This is 255 for normal releases.
	uint8_t bootloaderPrerelease;
	uint8_t bootloaderBuildType;
	uint8_t justActivated : 1;
	uint8_t updateError : 1;
} __attribute__((packed, aligned(4))) bluenet_ipc_bootloader_data_t;

/**
 * For now only running or not running is stored as process information.
 */
typedef struct {
	uint8_t running : 1;
} __attribute__((packed, aligned(4))) microapp_reboot_data_t;

/**
 * Data struct that can be used to communicate from bluenet to bluenet across reboots.
 */
typedef struct {
	// Major version of the data in this struct
	uint8_t ipcDataMajor;
	// Minor version of the data in this struct
	uint8_t ipcDataMinor;
	// Currently only 1 app is supported, expand with more bits for more apps
	microapp_reboot_data_t microapp[1];
} __attribute__((packed, aligned(4))) bluenet_ipc_bluenet_data_t;

/**
 * Make data available as union.
 */
typedef union {
	// Raw data
	uint8_t raw[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	// The data coming from the bootloader
	bluenet_ipc_bootloader_data_t bootloaderData;
	// The data from bluenet to bluenet firmware
	bluenet_ipc_bluenet_data_t bluenetRebootData;
} __attribute__((packed, aligned(4))) bluenet_ipc_data_t;

#ifdef __cplusplus
}
#endif
