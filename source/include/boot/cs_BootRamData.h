#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char index;
	char data[12];
	unsigned int checksum;
} __attribute__((packed, aligned(4))) boot_ram_data_t;

void setRamData(char* data, unsigned char length);

int getRamData(char* data, unsigned char length);

boot_ram_data_t *getRamStruct();

#ifdef __cplusplus
}
#endif
