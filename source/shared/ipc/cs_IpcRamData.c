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
bluenet_ipc_ram_data_t m_bluenet_ipc_ram __attribute__((section(".bluenet_ipc_ram"))) __attribute__((used));

/*
 * A simple additive checksum with inversion of the result to detect all zeros. This is the checksum used in IP
 * headers. The only difference is that here we run it over 8-bit data items rather than 16-bit words.
 */
uint16_t calculateChecksum(bluenet_ipc_ram_data_item_t* item) {
	uint16_t sum = 0;

	sum += item->header.major;
	sum += item->header.minor;
	sum += item->header.index;
	sum += item->header.dataSize;

	for (uint8_t i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		sum += item->data.raw[i];
	}
	sum = (sum >> 8) + (sum & 0xFF);
	sum += sum >> 8;

	return ~sum;
}

/*
 * We will do a memcpy with zero padding. This data has to be absolutely valid in all circumstances.
 */
enum IpcRetCode setRamData(bluenet_ipc_data_header_t* header, uint8_t* data) {
	if (data == NULL) {
		return IPC_RET_NULL_POINTER;
	}
	uint8_t index = header->index;
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (header->dataSize > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		return IPC_RET_DATA_TOO_LARGE;
	}
	// Copy header
	memcpy(&m_bluenet_ipc_ram.item[index].header, header, sizeof(header));
	// Copy data
	memcpy(m_bluenet_ipc_ram.item[index].data.raw, data, header->dataSize);
	// Zero padding of the data
	memset(m_bluenet_ipc_ram.item[index].data.raw + header->dataSize,
		   0,
		   BLUENET_IPC_RAM_DATA_ITEM_SIZE - header->dataSize);
	// Calculate the checksum ourselves (if present in header parameter it is ignored)
	m_bluenet_ipc_ram.item[index].header.checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
	return IPC_RET_SUCCESS;
}

/*
 * Allow receiving entity to obtain information about the header, such as minor and major to set up code that can be
 * backwards compatible. It is up to the receiving entity to indicate if the checksum has to be taken into account.
 *
 * For example, a reason to bump the major version is when the checksum calculation itself changes. This function will
 * in that case allow the developer to retrieve the (hopefully uncorrupted) major field and can call getRamData with
 * another major that it also knows how to parse.
 */
enum IpcRetCode getRamDataHeader(bluenet_ipc_data_header_t* header, bool use_checksum) {
	uint8_t index = header->index;
	if (header->index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (use_checksum) {
		uint16_t checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
		if (checksum != m_bluenet_ipc_ram.item[index].header.checksum) {
			return IPC_RET_DATA_INVALID;
		}
	}
	memcpy(header, &m_bluenet_ipc_ram.item[index].header, sizeof(header));
	return IPC_RET_SUCCESS;
}

/*
 * Note that we do not completely trust the data within RAM either...
 *
 * From the fields in header, the field index is expected to be written. If major and minor are both 0, they will be
 * filled with default values.
 *
 * If data is corrupted in this section, which can happen in all kinds of ways, from cosmic rays till stack overflows,
 * this can lead to segfaults. Hence, we are excessively checking stuff here.
 *
 * Ram data will not be returned if the header->major and header->minor arguments are lower than what is stored.
 */
enum IpcRetCode getRamData(bluenet_ipc_data_header_t* header, uint8_t* data, uint8_t max_length) {
	if (data == NULL) {
		return IPC_RET_NULL_POINTER;
	}
	uint8_t index = header->index;
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (header->major == 0 && header->minor == 0) {
		header->major = BLUENET_IPC_MAJOR_DEFAULT;
		header->minor = BLUENET_IPC_MINOR_DEFAULT;
	}
	if (m_bluenet_ipc_ram.item[index].header.index != index) {
		return IPC_RET_NOT_FOUND;
	}
	if (m_bluenet_ipc_ram.item[index].header.dataSize > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		return IPC_RET_DATA_TOO_LARGE;
	}
	if (max_length < m_bluenet_ipc_ram.item[index].header.dataSize) {
		return IPC_RET_BUFFER_TOO_SMALL;
	}
	if (header->major < m_bluenet_ipc_ram.item[index].header.major) {
		return IPC_RET_DATA_MAJOR_DIFF;
	}
	if (header->minor < m_bluenet_ipc_ram.item[index].header.minor) {
		return IPC_RET_DATA_MINOR_DIFF;
	}

	uint16_t checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
	if (checksum != m_bluenet_ipc_ram.item[index].header.checksum) {
		return IPC_RET_DATA_INVALID;
	}

	memcpy(data, &m_bluenet_ipc_ram.item[index].data, m_bluenet_ipc_ram.item[index].header.dataSize);
	header->dataSize = m_bluenet_ipc_ram.item[index].header.dataSize;
	return IPC_RET_SUCCESS;
}

bluenet_ipc_ram_data_item_t* getRamStruct(uint8_t index) {
	return &m_bluenet_ipc_ram.item[index];
}
