#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char g_FIRMWARE_VERSION[];

extern const char g_COMPILATION_DAY[];

extern const char g_BUILD_TYPE[];

extern const char g_BEACON_UUID[];

extern const uint16_t g_BEACON_MAJOR;

extern const uint16_t g_BEACON_MINOR;

extern const uint8_t g_BEACON_RSSI;

extern uint32_t g_APPLICATION_START_ADDRESS;

extern uint32_t g_APPLICATION_LENGTH;

extern uint32_t g_RAM_R1_BASE;

extern uint32_t g_RAM_APPLICATION_AMOUNT;

extern uint32_t g_HARDWARE_BOARD_ADDRESS;

extern uint32_t g_DEFAULT_HARDWARE_BOARD;

extern uint32_t g_FLASH_MICROAPP_BASE;

extern uint8_t g_FLASH_MICROAPP_PAGES;

extern uint32_t g_RAM_MICROAPP_BASE;

extern uint32_t g_RAM_MICROAPP_AMOUNT;

#ifdef __cplusplus
}
#endif

