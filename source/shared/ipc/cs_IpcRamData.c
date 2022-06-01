/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 18, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ipc/cs_IpcRamData.h>

#include <string.h>

/*
 * The interprocess communication (IPC) data in RAM is defined by the linker to reside on a particular spot in memory.
 * The buffer itself is kept small. It must be seen as the place where a list of functions can be stored, rather than
 * the data itself.
 */
bluenet_ipc_ram_data_t m_bluenet_ipc_ram
    __attribute__((section(".bluenet_ipc_ram")))
    __attribute__((used));

/*
 * A simple additive checksum with inversion of the result to detect
 * all zeros. This is the checksum used in IP headers. The only difference
 * is that here we run it over 8-bit data items rather than 16-bit words.
 */
uint16_t calculateChecksum(bluenet_ipc_ram_data_item_t * item) {
	uint16_t sum = 0;

	sum += item->index;
	sum += item->dataSize;

	for (uint8_t i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		sum += item->data[i];
	}
	sum = (sum >> 8) + (sum & 0xFF);
	sum += sum >> 8;

	return ~sum;
}

/*
 * We will do a memcpy with zero padding. This data has to be absolutely valid in all circumstances.
 */
enum IpcRetCode setRamData(uint8_t index, uint8_t* data, const uint8_t dataSize) {
	if (data == NULL) {
		return IPC_RET_NULL_POINTER;
	}
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (dataSize > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		return IPC_RET_DATA_TOO_LARGE;
	}
	m_bluenet_ipc_ram.item[index].index = index;
	m_bluenet_ipc_ram.item[index].dataSize = dataSize;

	memcpy(m_bluenet_ipc_ram.item[index].data, data, dataSize);

	// Zero padding
	memset(m_bluenet_ipc_ram.item[index].data + dataSize, 0, BLUENET_IPC_RAM_DATA_ITEM_SIZE - dataSize);

	m_bluenet_ipc_ram.item[index].checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
	return IPC_RET_SUCCESS;
}

enum IpcRetCode getRamData(uint8_t index, uint8_t* buf, uint8_t length, uint8_t* dataSize) {
	if (buf == NULL) {
		return IPC_RET_NULL_POINTER;
	}
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (m_bluenet_ipc_ram.item[index].index != index) {
		return IPC_RET_NOT_FOUND;
	}
	if (m_bluenet_ipc_ram.item[index].dataSize > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		return IPC_RET_DATA_TOO_LARGE;
	}
	if (length < m_bluenet_ipc_ram.item[index].dataSize) {
		return IPC_RET_BUFFER_TOO_SMALL;
	}

	uint16_t checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
	if (checksum != m_bluenet_ipc_ram.item[index].checksum) {
		return IPC_RET_DATA_INVALID;
	}
	
	memcpy(buf, m_bluenet_ipc_ram.item[index].data, m_bluenet_ipc_ram.item[index].dataSize);
	*dataSize = m_bluenet_ipc_ram.item[index].dataSize;
	return IPC_RET_SUCCESS;
}

/**
 * Clear everything, even the index itself.
 */
enum IpcRetCode clearRamData(uint8_t index) {
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	m_bluenet_ipc_ram.item[index].index = 0;
	m_bluenet_ipc_ram.item[index].dataSize = 0;
	memset(m_bluenet_ipc_ram.item[index].data, 0, BLUENET_IPC_RAM_DATA_ITEM_SIZE);
	return IPC_RET_SUCCESS;
}

bluenet_ipc_ram_data_item_t *getRamStruct(uint8_t index) {
	return &m_bluenet_ipc_ram.item[index];
}
