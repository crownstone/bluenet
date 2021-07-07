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

extern const int8_t g_BEACON_RSSI;

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

extern int8_t g_MAX_CHIP_TEMPERATURE;

extern uint16_t g_BOOT_DELAY;

extern uint16_t g_SCAN_DURATION;

extern uint16_t g_SCAN_BREAK_DURATION;

extern const int8_t g_TX_POWER;

extern const uint32_t g_CONNECTION_ALIVE_TIMEOUT;

extern const uint16_t g_MASTER_BUFFER_SIZE;

extern const bool g_CHANGE_NAME_ON_RESET;

extern const bool g_CONFIG_ENCRYPTION_ENABLED_DEFAULT;

extern const bool g_CONFIG_IBEACON_ENABLED_DEFAULT;

extern const bool g_CONFIG_MESH_ENABLED_DEFAULT;

extern const bool g_CONFIG_SCANNER_ENABLED_DEFAULT;

extern const bool g_CONFIG_TAP_TO_TOGGLE_ENABLED_DEFAULT;

extern const bool g_CONFIG_SWITCHCRAFT_ENABLED_DEFAULT;

extern const bool g_CONFIG_SWITCH_LOCK_ENABLED_DEFAULT;

extern const bool g_CONFIG_START_DIMMER_ON_ZERO_CROSSING_DEFAULT;

extern const bool g_CONFIG_DIMMING_ALLOWED_DEFAULT;

extern const int8_t g_CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_OFFSET_DEFAULT;

extern const uint8_t g_CONFIG_SPHERE_ID_DEFAULT;

extern const uint8_t g_CONFIG_CROWNSTONE_ID_DEFAULT;

extern const uint16_t g_CONFIG_RELAY_HIGH_DURATION_DEFAULT;

extern const int8_t g_CONFIG_LOW_TX_POWER_DEFAULT;

extern const uint8_t g_CS_SERIAL_ENABLED;

extern const bool g_ENABLE_RSSI_FOR_CONNECTION;

extern const uint8_t g_TWI_SCL_INDEX;

extern const uint8_t g_TWI_SDA_INDEX;

extern const bool g_AUTO_ENABLE_MICROAPP_ON_BOOT;

extern const int8_t g_GPIO_PIN1_INDEX;
extern const int8_t g_GPIO_PIN2_INDEX;
extern const int8_t g_GPIO_PIN3_INDEX;
extern const int8_t g_GPIO_PIN4_INDEX;

extern const int8_t g_BUTTON1_INDEX;
extern const int8_t g_BUTTON2_INDEX;
extern const int8_t g_BUTTON3_INDEX;
extern const int8_t g_BUTTON4_INDEX;

extern const int8_t g_LED1_INDEX;
extern const int8_t g_LED2_INDEX;
extern const int8_t g_LED3_INDEX;
extern const int8_t g_LED4_INDEX;

#ifdef __cplusplus
}
#endif

