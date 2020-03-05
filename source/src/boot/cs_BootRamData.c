#include <boot/cs_BootRamData.h>
#include <string.h>

boot_ram_data_t m_boot_ram  
    __attribute__((section(".bootloader_ram")))
    __attribute__((used));

void setRamData(char* data, unsigned char length) {
	m_boot_ram.index = 1;
	memcpy(m_boot_ram.data, data, length);
	// TODO: checksum 
}

int getRamData(char* data, unsigned char length) {
	// TODO: checksum 
	//LOGi("Search in address %p", m_boot_ram);

	if (m_boot_ram.index == 1) {
		memcpy(data, m_boot_ram.data, 12);
		data[11] = 0;
		return 1;
	}
	return 0;
}

boot_ram_data_t *getRamStruct() {
	return &m_boot_ram;
}
