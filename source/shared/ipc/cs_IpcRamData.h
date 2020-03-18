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

#define BLUENET_IPC_RAM_DATA_ITEM_SIZE                  24
#define BLUENET_IPC_RAM_DATA_ITEMS                      5

enum IpcRetCode {
	IPC_SUCCESS = 0,
	IPC_INDEX_OUT_OF_BOUND = 1,
	IPC_DATA_TOO_LARGE = 2,
	IPC_BUFFER_TOO_SMALL = 3,
	IPC_NOT_FOUND = 4,
	IPC_NULL_POINTER = 5,
};

/**
 * One item of data in the IPC ram.
 * The data array is word aligned.
 *
 * index      Has to matches the index of this item in the ram data.
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
enum IpcRetCode setRamData(uint8_t index, uint8_t* data, uint8_t dataSize);

/**
 * Get data from IPC ram.
 *
 * @param[in] index          Index of item.
 * @param[out] data          Buffer to copy the data to.
 * @param[in] length         Size of the buffer.
 * @param[out] dataSize      Size of the data.
 */
enum IpcRetCode getRamData(uint8_t index, uint8_t* buf, uint8_t length, uint8_t* dataSize);

bluenet_ipc_ram_data_item_t *getRamStruct(uint8_t index);

#ifdef __cplusplus
}
#endif
