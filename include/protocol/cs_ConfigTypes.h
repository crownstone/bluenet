/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 8, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventTypes.h>
#include <cfg/cs_Config.h>
#include <ble/cs_Nordic.h>

/** Configuration types
 *
 * Use in the characteristic to read and write configurations in <CommonService>.
 */
enum ConfigurationTypes {
    /*
	CONFIG_NAME                             = Configuration_Base,
	CONFIG_DEVICE_TYPE                      = 1,      //! 0x01
	CONFIG_ROOM                             = 2,      //! 0x02
	CONFIG_FLOOR                            = 3,      //! 0x03
	CONFIG_NEARBY_TIMEOUT                   = 4,      //! 0x04
	CONFIG_PWM_PERIOD                       = 5,      //! 0x05
	CONFIG_IBEACON_MAJOR                    = 6,      //! 0x06
	CONFIG_IBEACON_MINOR                    = 7,      //! 0x07
	CONFIG_IBEACON_UUID                     = 8,      //! 0x08
	CONFIG_IBEACON_TXPOWER                  = 9,      //! 0x09
//	CONFIG_WIFI_SETTINGS                    = 10,     //! 0x0A
	CONFIG_TX_POWER                         = 11,     //! 0x0B
	CONFIG_ADV_INTERVAL                     = 12,     //! 0x0C
	CONFIG_PASSKEY                          = 13,     //! 0x0D
	CONFIG_MIN_ENV_TEMP                     = 14,     //! 0x0E
	CONFIG_MAX_ENV_TEMP                     = 15,     //! 0x0F
	CONFIG_SCAN_DURATION                    = 16,     //! 0x10
	CONFIG_SCAN_SEND_DELAY                  = 17,     //! 0x11
	CONFIG_SCAN_BREAK_DURATION              = 18,     //! 0x12
	CONFIG_BOOT_DELAY                       = 19,     //! 0x13
	CONFIG_MAX_CHIP_TEMP                    = 20,     //! 0x14
	CONFIG_SCAN_FILTER                      = 21,     //! 0x15
	CONFIG_SCAN_FILTER_SEND_FRACTION        = 22,     //! 0x16
//	CONFIG_CURRENT_LIMIT                    = 23,     //! 0x17
	CONFIG_MESH_ENABLED                     = 24,     //! 0x18
	CONFIG_ENCRYPTION_ENABLED               = 25,     //! 0x19
	CONFIG_IBEACON_ENABLED                  = 26,     //! 0x1A
	CONFIG_SCANNER_ENABLED                  = 27,     //! 0x1B
	CONFIG_CONT_POWER_SAMPLER_ENABLED       = 28,     //! 0x1C
	CONFIG_TRACKER_ENABLED                  = 29,     //! 0x1D
//	CONFIG_ADC_BURST_SAMPLE_RATE            = 30,     //! 0x1E
//	CONFIG_POWER_SAMPLE_BURST_INTERVAL      = 31,     //! 0x1F
//	CONFIG_POWER_SAMPLE_CONT_INTERVAL       = 32,     //! 0x20
//	CONFIG_ADC_CONT_SAMPLE_RATE             = 33,     //! 0x1E
	CONFIG_CROWNSTONE_ID                    = 34,     //! 0x22
	CONFIG_KEY_ADMIN                        = 35,     //! 0x23
	CONFIG_KEY_MEMBER                       = 36,     //! 0x24
	CONFIG_KEY_GUEST                        = 37,     //! 0x25
	CONFIG_DEFAULT_ON                       = 38,     //! 0x26
	CONFIG_SCAN_INTERVAL                    = 39,     //! 0x27
	CONFIG_SCAN_WINDOW                      = 40,     //! 0x28
	CONFIG_RELAY_HIGH_DURATION              = 41,     //! 0x29
	CONFIG_LOW_TX_POWER                     = 42,     //! 0x2A
	CONFIG_VOLTAGE_MULTIPLIER               = 43,     //! 0x2B
	CONFIG_CURRENT_MULTIPLIER               = 44,     //! 0x2C
	CONFIG_VOLTAGE_ZERO                     = 45,     //! 0x2D
	CONFIG_CURRENT_ZERO                     = 46,     //! 0x2E
	CONFIG_POWER_ZERO                       = 47,     //! 0x2F
	CONFIG_POWER_ZERO_AVG_WINDOW            = 48,     //! 0x30
	CONFIG_MESH_ACCESS_ADDRESS              = 49,     //! 0x31
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD      = 50,     //! 0x32
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM  = 51,     //! 0x33
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP    = 52,     //! 0x34
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN  = 53,     //! 0x35
	CONFIG_PWM_ALLOWED                      = 54,     //! 0x36
	CONFIG_SWITCH_LOCKED                    = 55,     //! 0x37
	CONFIG_SWITCHCRAFT_ENABLED              = 56,     //! 0x38
	CONFIG_SWITCHCRAFT_THRESHOLD            = 57,     //! 0x39
	CONFIG_MESH_CHANNEL                     = 58,     //! 0x3A
	CONFIG_UART_ENABLED                     = 59,     //! 0x3B
*/
	CONFIG_TYPES
}; // Current max is 127 (0x7F), see cs_EventTypes.h


/**
 * For each type define the proper typedef so we can use strong typing in our functions.
 * Use the TYPIFY macro rather than typing CONFIG_ROOM_TYPE etc. explicitly.
 * The size of the type can be easily obtained through:
 *   sizeof(TYPIFY(CONFIG_ROOM)).
 */

#ifndef TYPIFY
#define TYPIFY(NAME) NAME ## _TYPE 
#endif
/*
typedef  uint8_t TYPIFY(CONFIG_SCAN_FILTER);
typedef  uint8_t TYPIFY(CONFIG_FLOOR);
typedef  uint8_t TYPIFY(CONFIG_MESH_CHANNEL);
typedef  uint8_t TYPIFY(CONFIG_UART_ENABLED); 

typedef   int8_t TYPIFY(CONFIG_LOW_TX_POWER);
typedef   int8_t TYPIFY(CONFIG_MAX_CHIP_TEMP);
typedef   int8_t TYPIFY(CONFIG_MAX_ENV_TEMP);
typedef   int8_t TYPIFY(CONFIG_MIN_ENV_TEMP);
typedef   int8_t TYPIFY(CONFIG_TX_POWER);
typedef   int8_t TYPIFY(CONFIG_IBEACON_TXPOWER); 
	
typedef uint16_t TYPIFY(CONFIG_SCAN_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_SCAN_WINDOW);
typedef uint16_t TYPIFY(CONFIG_RELAY_HIGH_DURATION);
typedef uint16_t TYPIFY(CONFIG_CROWNSTONE_ID);
typedef uint16_t TYPIFY(CONFIG_SCAN_FILTER_SEND_FRACTION);
typedef uint16_t TYPIFY(CONFIG_BOOT_DELAY);
typedef uint16_t TYPIFY(CONFIG_SCAN_BREAK_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_SEND_DELAY);
typedef uint16_t TYPIFY(CONFIG_ADV_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MINOR);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MAJOR);
typedef uint16_t TYPIFY(CONFIG_NEARBY_TIMEOUT);
typedef uint16_t TYPIFY(CONFIG_POWER_ZERO_AVG_WINDOW);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM); 
	
typedef uint32_t TYPIFY(CONFIG_PWM_PERIOD);
typedef uint32_t TYPIFY(CONFIG_MESH_ACCESS_ADDRESS); 

typedef  int32_t TYPIFY(CONFIG_VOLTAGE_ZERO);
typedef  int32_t TYPIFY(CONFIG_CURRENT_ZERO);
typedef  int32_t TYPIFY(CONFIG_POWER_ZERO); 

typedef    float TYPIFY(CONFIG_VOLTAGE_MULTIPLIER);
typedef    float TYPIFY(CONFIG_CURRENT_MULTIPLIER);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN);
typedef    float TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD); 

typedef     bool TYPIFY(CONFIG_CONT_POWER_SAMPLER_ENABLED);
typedef     bool TYPIFY(CONFIG_DEFAULT_ON);
typedef     bool TYPIFY(CONFIG_ENCRYPTION_ENABLED);
typedef     bool TYPIFY(CONFIG_IBEACON_ENABLED);
typedef     bool TYPIFY(CONFIG_MESH_ENABLED);
typedef     bool TYPIFY(CONFIG_PWM_ALLOWED);
typedef     bool TYPIFY(CONFIG_SCANNER_ENABLED);
typedef     bool TYPIFY(CONFIG_SWITCH_LOCKED);
typedef     bool TYPIFY(CONFIG_SWITCHCRAFT_ENABLED);
typedef     bool TYPIFY(CONFIG_TRACKER_ENABLED);

const uint8_t ConfigurationTypeSizes [] = {
    MAX_STRING_STORAGE_SIZE+1, // CONFIG_NAME
    0, // CONFIG_DEVICE_TYPE
    0, // CONFIG_ROOM
    sizeof(TYPIFY(CONFIG_FLOOR)), 
    sizeof(TYPIFY(CONFIG_NEARBY_TIMEOUT)),
    sizeof(TYPIFY(CONFIG_PWM_PERIOD)),
    sizeof(TYPIFY(CONFIG_IBEACON_MAJOR)), 
    sizeof(TYPIFY(CONFIG_IBEACON_MINOR)),
    16, // CONFIG_IBEACON_UUID
    sizeof(TYPIFY(CONFIG_IBEACON_TXPOWER)),
    0, // CONFIG_WIFI_SETTINGS
    sizeof(TYPIFY(CONFIG_TX_POWER)),
    sizeof(TYPIFY(CONFIG_ADV_INTERVAL)),
    BLE_GAP_PASSKEY_LEN, // CONFIG_PASSKEY
    sizeof(TYPIFY(CONFIG_MIN_ENV_TEMP)),
    sizeof(TYPIFY(CONFIG_MAX_ENV_TEMP)),
    sizeof(TYPIFY(CONFIG_SCAN_DURATION)),
    sizeof(TYPIFY(CONFIG_SCAN_SEND_DELAY)),
    sizeof(TYPIFY(CONFIG_SCAN_BREAK_DURATION)),
    sizeof(TYPIFY(CONFIG_BOOT_DELAY)),
    sizeof(TYPIFY(CONFIG_MAX_CHIP_TEMP)),
    sizeof(TYPIFY(CONFIG_SCAN_FILTER)),
    sizeof(TYPIFY(CONFIG_SCAN_FILTER_SEND_FRACTION)),
    0, // CONFIG_CURRENT_LIMIT)),
    sizeof(TYPIFY(CONFIG_MESH_ENABLED)),
    sizeof(TYPIFY(CONFIG_ENCRYPTION_ENABLED)),
    sizeof(TYPIFY(CONFIG_IBEACON_ENABLED)),
    sizeof(TYPIFY(CONFIG_SCANNER_ENABLED)),
    sizeof(TYPIFY(CONFIG_CONT_POWER_SAMPLER_ENABLED)),
    sizeof(TYPIFY(CONFIG_TRACKER_ENABLED)),
    0, // CONFIG_ADC_BURST_SAMPLE_RATE)),
    0, // CONFIG_POWER_SAMPLE_BURST_INTERVAL)),
    0, // CONFIG_POWER_SAMPLE_CONT_INTERVAL)),
    0, // CONFIG_ADC_CONT_SAMPLE_RATE)),
    sizeof(TYPIFY(CONFIG_CROWNSTONE_ID)),
    ENCRYPTION_KEY_LENGTH, // CONFIG_KEY_ADMIN
    ENCRYPTION_KEY_LENGTH, // CONFIG_KEY_MEMBER
    ENCRYPTION_KEY_LENGTH, // CONFIG_KEY_GUEST
    sizeof(TYPIFY(CONFIG_DEFAULT_ON)),
    sizeof(TYPIFY(CONFIG_SCAN_INTERVAL)),
    sizeof(TYPIFY(CONFIG_SCAN_WINDOW)),
    sizeof(TYPIFY(CONFIG_RELAY_HIGH_DURATION)),
    sizeof(TYPIFY(CONFIG_LOW_TX_POWER)),
    sizeof(TYPIFY(CONFIG_VOLTAGE_MULTIPLIER)),
    sizeof(TYPIFY(CONFIG_CURRENT_MULTIPLIER)),
    0, // CONFIG_VOLTAGE_ZERO)),
    sizeof(TYPIFY(CONFIG_CURRENT_ZERO)),
    sizeof(TYPIFY(CONFIG_POWER_ZERO)),
    sizeof(TYPIFY(CONFIG_POWER_ZERO_AVG_WINDOW)),
    0, // CONFIG_MESH_ACCESS_ADDRESS)),
    sizeof(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD)),
    sizeof(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM)),
    sizeof(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP)),
    sizeof(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN)),
    sizeof(TYPIFY(CONFIG_PWM_ALLOWED)),
    sizeof(TYPIFY(CONFIG_SWITCH_LOCKED)),
    sizeof(TYPIFY(CONFIG_SWITCHCRAFT_ENABLED)),
    sizeof(TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD)),
    sizeof(TYPIFY(CONFIG_MESH_CHANNEL)),
    sizeof(TYPIFY(CONFIG_UART_ENABLED))
}; 

*/
extern char const *ConfigurationNames[];
