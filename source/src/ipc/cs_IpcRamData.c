#include <ipc/cs_IpcRamData.h>

#include <string.h>

bluenet_ipc_ram_data_t m_bluenet_ipc_ram
    __attribute__((section(".bluenet_ipc_ram")))
    __attribute__((used));

uint16_t calculateChecksum(bluenet_ipc_ram_data_item_t * item) {
	uint16_t sum = 0;

	sum += item->index;

	for (uint8_t i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		sum += item->data[i];
	}
	sum = (sum >> 8) + (sum & 0xFF);
	sum += sum >> 8;

	return ~sum;
}  

void setRamData(uint8_t index, uint8_t* data, uint8_t length) {
	// make sure to the index before calculating the checksum (in it includes the index)
	m_bluenet_ipc_ram.item[index].index = index;
	if (length < BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		// make null-terminated, just for convenience sake
		memcpy(m_bluenet_ipc_ram.item[index].data, data, length);
		m_bluenet_ipc_ram.item[index].data[length] = 0;
	} else {
		// take care of null-termination yourself
		memcpy(m_bluenet_ipc_ram.item[index].data, data, BLUENET_IPC_RAM_DATA_ITEM_SIZE);
	}
	m_bluenet_ipc_ram.item[index].checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
}

uint8_t getRamData(uint8_t index, uint8_t* data, uint8_t length) {
	//LOGi("Search at address %p", m_boot_ram);
	if (m_bluenet_ipc_ram.item[index].index != index) {
		return 1;
	}

	uint16_t checksum = calculateChecksum(&m_bluenet_ipc_ram.item[index]);
	if (checksum != m_bluenet_ipc_ram.item[index].checksum) {
		return 2;
	}
	
	memcpy(data, m_bluenet_ipc_ram.item[index].data, (length < BLUENET_IPC_RAM_DATA_ITEM_SIZE) ? length : BLUENET_IPC_RAM_DATA_ITEM_SIZE);

	return 0;
}

bluenet_ipc_ram_data_item_t *getRamStruct(uint8_t index) {
	return &m_bluenet_ipc_ram.item[index];
}
