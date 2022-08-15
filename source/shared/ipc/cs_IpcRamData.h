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

#include <stdint.h>

// The number of slots for interprocess communication
#define BLUENET_IPC_RAM_DATA_ITEMS 5

// The size of each slot for interprocess communication
#define BLUENET_IPC_RAM_DATA_ITEM_SIZE 24

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
	uint8_t protocol;  // Should be 1.
	uint16_t dfu_version;
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t prerelease;  // 255 means it's not a pre-release.
	uint8_t build_type;  // See BuildType.
} __attribute__((packed, aligned(4))) bluenet_ipc_bootloader_data_t;

/**
 * One item of data in the IPC ram.
 * The data array is word aligned.
 *
 * index      The index of this item, to see if this item has been set.
 * dataSize   How many bytes of useful data is in the data array.
 * checksum   Checksum calculated over the index, data size, and _complete_ data array.
 */
typedef struct {
	uint8_t index;
	uint8_t dataSize;
	uint16_t checksum;
	uint8_t data[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
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
 * @param[in] index          Index of item.
 * @param[in] data           Data pointer.
 * @param[in] dataSize       Size of the data.
 */
enum IpcRetCode setRamData(uint8_t index, uint8_t* data, const uint8_t dataSize);

/**
 * Get data from IPC ram.
 *
 * @param[in] index          Index of item.
 * @param[out] buf           Buffer to copy the data to.
 * @param[in] length         Size of the buffer.
 * @param[out] dataSize      Size of the data.
 */
enum IpcRetCode getRamData(uint8_t index, uint8_t* buf, uint8_t length, uint8_t* dataSize);

/**
 * Get the underlying complete data struct. Do not use if not truly necessary. Its implementation might change.
 */
bluenet_ipc_ram_data_item_t* getRamStruct(uint8_t index);

#ifdef __cplusplus
}
#endif
