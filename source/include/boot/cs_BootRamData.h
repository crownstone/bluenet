#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char index;
	char data[12];
	unsigned int checksum;
} boot_ram_data_t;

void setRamData(char* data, unsigned char length);

int getRamData(char* data, unsigned char length);

#ifdef __cplusplus
}
#endif
