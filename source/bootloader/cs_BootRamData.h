#pragma once

#include <stdint.h>

#define BOOT_RAM_DATA_ITEM_SIZE                  24
#define BOOT_RAM_DATA_ITEMS                      5

/**
 * Store an index, data array, and checksum
 */
typedef struct {
	char index;
	char data[BOOT_RAM_DATA_ITEM_SIZE];
	unsigned int checksum;
} __attribute__((packed, aligned(4))) boot_ram_data_item_t;

/**
 * Have a fixed 
 */
typedef struct {
	boot_ram_data_item_t item[BOOT_RAM_DATA_ITEMS];
} __attribute__((packed, aligned(4))) boot_ram_data_t;

void setChecksum(boot_ram_data_item_t * item);

void setRamData(char* data, unsigned char length);
