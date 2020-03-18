#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BLUENET_IPC_RAM_DATA_ITEM_SIZE                  24
#define BLUENET_IPC_RAM_DATA_ITEMS                      5

/**
 * Store an index, data array, and checksum
 */
typedef struct {
	uint8_t index;
	uint8_t data[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	uint16_t checksum;
} __attribute__((packed, aligned(4))) bluenet_ipc_ram_data_item_t;

/**
 * Have a fixed number of boot ram data items.
 */
typedef struct {
	bluenet_ipc_ram_data_item_t item[BLUENET_IPC_RAM_DATA_ITEMS];
} __attribute__((packed, aligned(4))) bluenet_ipc_ram_data_t;

/**
 */
uint16_t calculateChecksum(bluenet_ipc_ram_data_item_t * item);

void setRamData(uint8_t index, uint8_t* data, uint8_t length);

uint8_t getRamData(uint8_t index, uint8_t* data, uint8_t length);

bluenet_ipc_ram_data_item_t *getRamStruct(uint8_t index);

#ifdef __cplusplus
}
#endif
