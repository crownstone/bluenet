#include <cs_BootRamData.h>
#include <string.h>

boot_ram_data_t m_boot_ram  
    __attribute__((section(".bootloader_ram")))
    __attribute__((used));


void setChecksum(boot_ram_data_item_t * item) {
	uint16_t sum = 0;

	sum += item->index;

	for (uint8_t i = 0; i < BOOT_RAM_DATA_ITEM_SIZE; ++i) {
		sum += item->data[i];
	}
	sum = (sum >> 8) + (sum & 0xFF);
	sum += sum >> 8;

	item->checksum = ~sum;
}  

void setRamData(char* data, unsigned char length) {
	m_boot_ram.item[1].index = 1;
	memcpy(m_boot_ram.item[1].data, data, (length < BOOT_RAM_DATA_ITEM_SIZE ? length : BOOT_RAM_DATA_ITEM_SIZE) );
	setChecksum(&m_boot_ram.item[1]);
}
