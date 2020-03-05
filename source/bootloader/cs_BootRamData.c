#include <cs_BootRamData.h>
#include <string.h>

boot_ram_data_t m_boot_ram  
    __attribute__((section(".bootloader_ram")))
    __attribute__((used));

void setRamData(char* data, unsigned char length) {
	m_boot_ram.index = 0;
	memcpy(m_boot_ram.data, data, length);
	// TODO: checksum 
}
