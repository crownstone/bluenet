#include <boot/cs_BootRamData.h>
#include <string.h>

boot_ram_data_t m_boot_ram  
    __attribute__((section(".bootloader_ram")))
    __attribute__((used));

uint16_t calculateChecksum(boot_ram_data_item_t * item) {
	uint16_t sum = 0;

	sum += item->index;

	for (uint8_t i = 0; i < BOOT_RAM_DATA_ITEM_SIZE; ++i) {
		sum += item->data[i];
	}
	sum = (sum >> 8) + (sum & 0xFF);
	sum += sum >> 8;

	return ~sum;
}  

void setRamData(uint8_t index, uint8_t* data, uint8_t length) {
	// make sure to the index before calculating the checksum (in it includes the index)
	m_boot_ram.item[index].index = index;
	if (length < BOOT_RAM_DATA_ITEM_SIZE) {
		// make null-terminated, just for convenience sake
		memcpy(m_boot_ram.item[index].data, data, length);
		m_boot_ram.item[index].data[length] = 0;
	} else {
		// take care of null-termination yourself
		memcpy(m_boot_ram.item[index].data, data, BOOT_RAM_DATA_ITEM_SIZE);
	}
	m_boot_ram.item[index].checksum = calculateChecksum(&m_boot_ram.item[index]);
}

uint8_t getRamData(uint8_t index, uint8_t* data, uint8_t length) {
	//LOGi("Search at address %p", m_boot_ram);
	if (m_boot_ram.item[index].index != index) { 
		return 1;
	}

	uint16_t checksum = calculateChecksum(&m_boot_ram.item[index]);
	if (checksum != m_boot_ram.item[index].checksum) {
		return 2;
	}
	
	memcpy(data, m_boot_ram.item[index].data, (length < BOOT_RAM_DATA_ITEM_SIZE) ? length : BOOT_RAM_DATA_ITEM_SIZE);

	return 0;
}

boot_ram_data_item_t *getRamStruct(uint8_t index) {
	return &m_boot_ram.item[index];
}
