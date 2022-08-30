/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 18, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// The number of slots for interprocess communication
#define BLUENET_IPC_RAM_DATA_ITEMS 5

// The size of each slot for interprocess communication
#define BLUENET_IPC_RAM_DATA_ITEM_SIZE 24

// Version for major for the IPC header itself
#define BLUENET_IPC_HEADER_MAJOR 1

// Version for minor for the IPC header itself
#define BLUENET_IPC_HEADER_MINOR 0

enum IpcIndex {
	IPC_INDEX_RESERVED           = 0,
	IPC_INDEX_CROWNSTONE_APP     = 1,
	IPC_INDEX_BOOTLOADER_VERSION = 2,
	IPC_INDEX_MICROAPP           = 3,
};

enum IpcRetCode {
	IPC_RET_SUCCESS            = 0,
	IPC_RET_INDEX_OUT_OF_BOUND = 1,
	IPC_RET_DATA_TOO_LARGE     = 2,
	IPC_RET_BUFFER_TOO_SMALL   = 3,
	IPC_RET_NOT_FOUND          = 4,
	IPC_RET_NULL_POINTER       = 5,
	IPC_RET_DATA_INVALID       = 6,
	IPC_RET_DATA_MAJOR_DIFF    = 7,
	IPC_RET_DATA_MINOR_DIFF    = 8,
};

enum BuildType {
	BUILD_TYPE_RESERVED       = 0,
	BUILD_TYPE_DEBUG          = 1,
	BUILD_TYPE_RELEASE        = 2,
	BUILD_TYPE_RELWITHDEBINFO = 3,
	BUILD_TYPE_MINSIZEREL     = 4,
};

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
	// Major version of this struct
	uint8_t ipcDataMajor;
	uint8_t ipcDataMinor;
	uint16_t dfuVersion;
	uint8_t bootloaderMajor;
	uint8_t bootloaderMinor;
	uint8_t bootloaderPatch;
	// A prerelease value. This is 255 for normal releases.
	uint8_t bootloaderPrerelease;
	uint8_t bootloaderBuildType;
} __attribute__((packed, aligned(4))) bluenet_ipc_bootloader_data_t;

/**
 * Make data available as union.
 */
typedef union {
	// Raw data
	uint8_t raw[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	// The data coming from the bootloader
	bluenet_ipc_bootloader_data_t bootloaderData;
} __attribute__((packed, aligned(4))) bluenet_ipc_data_t;

/**
 * The header of a data item in IPC ram.
 *
 * Bump the major version if there's a change that is not backwards-compatible. Bump the minor version if there's a
 * change that is backwards-compatible, for example the addition of a field within the data object.
 */
typedef struct {
	// The major version.
	uint8_t major;
	// The minor version.
	uint8_t minor;
	// The index of this item, to see if this item has been set.
	uint8_t index;
	// How many bytes are informational within the data array.
	uint8_t dataSize;
	// Checksum calculated over the data array, but also all fields above.
	uint16_t checksum;
	// Reserve some bytes to be word aligned.
	uint8_t reserved[2];
} __attribute__((packed, aligned(4))) bluenet_ipc_data_header_t;

/**
 * One item of data in the IPC ram. The data is word-aligned.
 */
typedef struct {
	// The header
	bluenet_ipc_data_header_t header;
	// The data array
	bluenet_ipc_data_t data;
} __attribute__((packed, aligned(4))) bluenet_ipc_ram_data_item_t;

/**
 * Have a fixed number of IPC ram data items.
 */
typedef struct {
	bluenet_ipc_ram_data_item_t item[BLUENET_IPC_RAM_DATA_ITEMS];
} __attribute__((packed, aligned(4))) bluenet_ipc_ram_data_t;

/**
 * Set data in IPC ram.
 *
 * @param[in] index          Index of IPC segment.
 * @param[in] dataSize       Size of data.
 * @param[in] data           Data pointer.
 * @return                   Error code (success is indicated by 0).
 */
enum IpcRetCode setRamData(uint8_t index, uint8_t dataSize, uint8_t* data);

/**
 * Get data from IPC ram.
 *
 * @param[in] index          Index of IPC segment.
 * @param[out] data          Buffer to copy the data to.
 * @param[out] dataSize      Size of data.
 * @param[in] maxSize        Size of the buffer to copy the data to (should be large enough).
 * @return                   Error code (success is indicated by 0).
 */
enum IpcRetCode getRamData(uint8_t index, uint8_t* data, uint8_t* dataSize, uint8_t maxSize);

/**
 * Get header for specific data. Calculating the checksum can be omitted.
 *
 * @param[out] header              Get only the header.
 * @param[in] index                Index of IPC segment.
 * @param[in] doCalculateChecksum  Calculate the checksum.
 * @return                         Error code (success is indicated by 0).
 */
enum IpcRetCode getRamDataHeader(bluenet_ipc_data_header_t* header, uint8_t index, bool doCalculateChecksum);

/**
 * Get the underlying complete data struct. Do not use if not truly necessary. Its implementation might change.
 */
bluenet_ipc_ram_data_item_t* getRamStruct(uint8_t index);

#ifdef __cplusplus
}
#endif
