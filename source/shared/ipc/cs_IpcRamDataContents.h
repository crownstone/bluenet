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

enum BuildType {
	BUILD_TYPE_RESERVED       = 0,
	BUILD_TYPE_DEBUG          = 1,
	BUILD_TYPE_RELEASE        = 2,
	BUILD_TYPE_RELWITHDEBINFO = 3,
	BUILD_TYPE_MINSIZEREL     = 4,
};

/**
 * The protocol struct with which the bootloader communicates to the bluenet binary through RAM.
 */
typedef struct {
	//! The version of the data in this struct.
	uint8_t ipcDataMajor;
	uint8_t ipcDataMinor;

	//! The DFU version is increased by one every release.
	uint16_t dfuVersion;

	/**
	 * The semantic version, for example "2.1.0".
	 * This is used to check for compatibility and for the firmware update processes.
	 */
	uint8_t bootloaderMajor;
	uint8_t bootloaderMinor;
	uint8_t bootloaderPatch;

	/**
	 * A prerelease value, used for release candidates. This is 255 for normal releases.
	 * For example: if the prelease is 3, the version would be "2.1.0-RC3".
	 */
	uint8_t bootloaderPrerelease;

	//! See BuildType.
	uint8_t bootloaderBuildType;

	union {
		struct {
			//! When true, this is the first time this application version is running.
			uint8_t justActivated : 1;

			//! When true there was an IPC version that could not be parsed by the bootloader.
			uint8_t versionError : 1;

			//! When the bootloader failed to get ram data. This is normal on a cold boot.
			uint8_t readError : 1;
		} __attribute__((packed)) flags;
		uint8_t flagsRaw;
	};
} __attribute__((packed)) bluenet_ipc_bootloader_data_t;

/**
 * For now only running or not running is stored as process information.
 */
typedef struct {
	uint8_t running : 1;
} __attribute__((packed)) microapp_reboot_data_t;

/**
 * Data struct that can be used to communicate from bluenet to bluenet across reboots.
 */
typedef struct {
	//! Major version of the data in this struct
	uint8_t ipcDataMajor;
	//! Minor version of the data in this struct
	uint8_t ipcDataMinor;

	//! Energy used since last cold boot, in mJ.
	int64_t energyUsedMicroJoule;

	//! Currently only 1 app is supported, expand with more bits for more apps
	microapp_reboot_data_t microapp[1];

} __attribute__((packed)) bluenet_ipc_bluenet_data_t;

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
} __attribute__((packed)) bluenet_ipc_data_payload_t;

#ifdef __cplusplus
}
#endif
