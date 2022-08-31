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
	sum += item->header.reserved[0];
	sum += item->header.reserved[1];

	for (uint8_t i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		sum += item->data.raw[i];
	}
	sum = (sum >> 8) + (sum & 0xFF);
	sum += sum >> 8;

	return ~sum;
}

/*
 * We will do a memcpy with zero padding. This data has to be absolutely valid in all circumstances.
 *
 * Make sure header is initialized with zeros for unused fields when calling this function.
 */
enum IpcRetCode setRamData(uint8_t index, uint8_t* data, uint8_t dataSize) {
	if (data == NULL) {
		return IPC_RET_NULL_POINTER;
	}
	bluenet_ipc_data_header_t header;
	header.index       = index;
	header.dataSize    = dataSize;
	header.major       = BLUENET_IPC_HEADER_MAJOR;
	header.minor       = BLUENET_IPC_HEADER_MINOR;
	header.reserved[0] = 0;
	header.reserved[1] = 0;
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (dataSize > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		return IPC_RET_DATA_TOO_LARGE;
	}
	// Copy header
	memcpy(&m_bluenet_ipc_ram.item[index].header, &header, sizeof(header));
	// Copy data
	memcpy(m_bluenet_ipc_ram.item[index].data.raw, data, dataSize);
	// Zero padding of the data
	memset(m_bluenet_ipc_ram.item[index].data.raw + dataSize, 0, BLUENET_IPC_RAM_DATA_ITEM_SIZE - dataSize);
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
enum IpcRetCode getRamDataHeader(bluenet_ipc_data_header_t* header, uint8_t index, bool doCalculateChecksum) {
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (doCalculateChecksum) {
		uint16_t checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
		if (checksum != m_bluenet_ipc_ram.item[index].header.checksum) {
			return IPC_RET_DATA_INVALID;
		}
	}
	memcpy(header, &m_bluenet_ipc_ram.item[index].header, sizeof(header));
	return IPC_RET_SUCCESS;
}

/*
 * To judge if RAM data is present we just assume that both major and minor are still zero. It doesn't tell if the item
 * is valid. We can't use other fields than major and minor because they can change.
 */
bool isRamDataPresent(uint8_t index) {
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return false;
	}
	return (!m_bluenet_ipc_ram.item[index].header.major && !m_bluenet_ipc_ram.item[index].header.minor);
}

/*
 * Returns true if the entire buffer including the header is empty.
 */
bool isRamDataEmpty(uint8_t index) {
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return false;
	}
	for (int i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		if (m_bluenet_ipc_ram.item[index].data.raw[i] != 0) {
			return false;
		}
	}
	return true;
}

/**
 * Clear RAM data
 */
enum IpcRetCode clearRamData(uint8_t index) {
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	memset(m_bluenet_ipc_ram.item[index].data.raw, 0, BLUENET_IPC_RAM_DATA_ITEM_SIZE);
	return IPC_RET_SUCCESS;
}

/*
 * From the fields in header, all fields are optional. The index field is not considered within the header because,
 * potentially, a version update might shift the index field.
 *
 * If major and minor are both 0, they will be filled with default values.
 *
 * If data is corrupted in this section, which can happen in all kinds of ways, from cosmic rays till stack overflows,
 * this can lead to segfaults. Hence, we are excessively checking stuff here.
 *
 * Ram data will not be returned for any different header->major and any lower header->minor argument. The sending
 * party can increase the minor when fields are added and current fields are preserved. In that way the minor can be
 * used for future compatibility. When the major is bumped consider communication with previous software to be
 * absent. It will only be possible to do an upgrade procedure where first the receiving party is able to receive
 * the new protocol and only after that has been rolled out the sending party can start to send it.
 *
 * Note that we do not completely trust the data within RAM either... The value header.dataSize is also checked for
 * example.
 *
 * Note also that dataSize is not checked. It is only used as return value. This allows updates to minor versions
 * that only expand the IPC header struct.
 */
enum IpcRetCode getRamData(uint8_t index, uint8_t* data, uint8_t* dataSize, uint8_t maxSize) {
	if (data == NULL) {
		return IPC_RET_NULL_POINTER;
	}
	if (index > BLUENET_IPC_RAM_DATA_ITEMS) {
		return IPC_RET_INDEX_OUT_OF_BOUND;
	}
	if (m_bluenet_ipc_ram.item[index].header.major != BLUENET_IPC_HEADER_MAJOR) {
		return IPC_RET_DATA_MAJOR_DIFF;
	}
	if (m_bluenet_ipc_ram.item[index].header.minor > BLUENET_IPC_HEADER_MINOR) {
		return IPC_RET_DATA_MINOR_DIFF;
	}
	if (m_bluenet_ipc_ram.item[index].header.index != index) {
		return IPC_RET_NOT_FOUND;
	}
	if (m_bluenet_ipc_ram.item[index].header.dataSize > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		return IPC_RET_DATA_TOO_LARGE;
	}
	if (m_bluenet_ipc_ram.item[index].header.dataSize > maxSize) {
		return IPC_RET_BUFFER_TOO_SMALL;
	}
	uint16_t checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
	if (checksum != m_bluenet_ipc_ram.item[index].header.checksum) {
		return IPC_RET_DATA_INVALID;
	}

	memcpy(data, m_bluenet_ipc_ram.item[index].data.raw, m_bluenet_ipc_ram.item[index].header.dataSize);
	*dataSize = m_bluenet_ipc_ram.item[index].header.dataSize;
	return IPC_RET_SUCCESS;
}

bluenet_ipc_ram_data_item_t* getRamStruct(uint8_t index) {
	return &m_bluenet_ipc_ram.item[index];
}
