/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 8, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>
#include <type_traits>

typedef uint16_t size16_t;

enum TypeBases {
	Configuration_Base = 0x000,
	State_Base         = 0x080,
	General_Base       = 0x100,
	Uart_Base          = 0x180,
};

template <typename T>
constexpr auto operator+(T e) noexcept 
-> std::enable_if_t<std::is_enum<T>::value, std::underlying_type_t<T>> {
   return static_cast<std::underlying_type_t<T>>(e);
}

/** State variable types
 * Also used as event types
 * Use in the characteristic to read and write state variables in <CommonService>.
 */
enum class CS_TYPE: uint16_t {
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

    //CONFIG_TYPES,
    
    STATE_RESET_COUNTER = State_Base,     // 0x80 - 128
    STATE_SWITCH_STATE,                   // 0x81 - 129
    STATE_ACCUMULATED_ENERGY,             // 0x82 - 130
    STATE_POWER_USAGE,                    // 0x83 - 131
    STATE_TRACKED_DEVICES,                // 0x84 - 132
    STATE_SCHEDULE,                       // 0x85 - 133
    STATE_OPERATION_MODE,                 // 0x86 - 134
    STATE_TEMPERATURE,                    // 0x87 - 135
    STATE_TIME,                           // 0x88 - 136
    STATE_FACTORY_RESET,                  // 0x89 - 137
    STATE_LEARNED_SWITCHES,               // 0x8A - 138
    STATE_ERRORS,                         // 0x8B - 139
    STATE_ERROR_OVER_CURRENT,             // 0x8C - 140
    STATE_ERROR_OVER_CURRENT_PWM,         // 0x8D - 141
    STATE_ERROR_CHIP_TEMP,                // 0x8E - 142
    STATE_ERROR_PWM_TEMP,                 // 0x8F - 143
    STATE_IGNORE_BITMASK,                 // 0x90 - 144
    STATE_IGNORE_ALL,                     // 0x91 - 145
    STATE_IGNORE_LOCATION,                // 0x92 - 146
    STATE_ERROR_DIMMER_ON_FAILURE,        // 0x93 - 147
    STATE_ERROR_DIMMER_OFF_FAILURE,       // 0x94 - 148

    //STATE_TYPES
    
    EVT_POWER_OFF = General_Base,
    EVT_POWER_ON,
    EVT_POWER_TOGGLE,
    EVT_ADVERTISEMENT_UPDATED,
    EVT_SCAN_STARTED,
    EVT_SCAN_STOPPED,
    EVT_SCANNED_DEVICES,
    EVT_DEVICE_SCANNED,
    EVT_POWER_SAMPLES_START,
    EVT_POWER_SAMPLES_END,
    EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM,
    EVT_CURRENT_USAGE_ABOVE_THRESHOLD,
    EVT_DIMMER_ON_FAILURE_DETECTED,
    EVT_DIMMER_OFF_FAILURE_DETECTED,
    //	EVT_POWER_CONSUMPTION,
    //	EVT_ENABLED_MESH,
    //	EVT_ENABLED_ENCRYPTION,
    //	EVT_ENABLED_IBEACON,
    //	EVT_CHARACTERISTICS_UPDATED,
    EVT_TRACKED_DEVICES,
    EVT_TRACKED_DEVICE_IS_NEARBY,
    EVT_TRACKED_DEVICE_NOT_NEARBY,
    EVT_MESH_TIME, // Sent when the mesh received the current time
    EVT_SCHEDULE_ENTRIES_UPDATED,
    EVT_BLE_EVENT,
    EVT_BLE_CONNECT,
    EVT_BLE_DISCONNECT,
    EVT_STATE_NOTIFICATION,
    EVT_BROWNOUT_IMPENDING,
    EVT_SESSION_NONCE_SET,
    EVT_KEEP_ALIVE,
    EVT_PWM_FORCED_OFF,
    EVT_SWITCH_FORCED_OFF,
    EVT_RELAY_FORCED_ON,
    EVT_CHIP_TEMP_ABOVE_THRESHOLD,
    EVT_CHIP_TEMP_OK,
    EVT_PWM_TEMP_ABOVE_THRESHOLD,
    EVT_PWM_TEMP_OK,
    EVT_EXTERNAL_STATE_MSG_CHAN_0,
    EVT_EXTERNAL_STATE_MSG_CHAN_1,
    EVT_TIME_SET, // Sent when the time has been set or changed.
    EVT_PWM_POWERED,
    EVT_PWM_ALLOWED, // Sent when pwm allowed flag is set. Payload is boolean.
    EVT_SWITCH_LOCKED, // Sent when switch locked flag is set. Payload is boolean.
    EVT_STORAGE_WRITE_DONE, // Sent when storage write is done and queue is empty.
    EVT_SETUP_DONE, // Sent when setup was done (and all settings have been stored).
    EVT_DO_RESET_DELAYED, // Sent to perform a reset in a few seconds (currently done by command handler). Payload is evt_do_reset_delayed_t.
    EVT_SWITCHCRAFT_ENABLED, // Sent when switchcraft flag is set. Payload is boolean.
    EVT_STORAGE_WRITE, // Sent when an item is going to be written to storage.
    EVT_STORAGE_ERASE, // Sent when a flash page is going to be erased.
    EVT_ADC_RESTARTED, // Sent when ADC has been restarted: the next buffer is expected to be different from the previous ones.

    EVT_SET_LOG_LEVEL = Uart_Base,
    EVT_ENABLE_LOG_POWER,
    EVT_ENABLE_LOG_CURRENT,
    EVT_ENABLE_LOG_VOLTAGE,
    EVT_ENABLE_LOG_FILTERED_CURRENT,
    EVT_CMD_RESET,
    EVT_ENABLE_ADVERTISEMENT,
    EVT_ENABLE_MESH,
    EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN,
    EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT,
    EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE,
    EVT_INC_VOLTAGE_RANGE,
    EVT_DEC_VOLTAGE_RANGE,
    EVT_INC_CURRENT_RANGE,
    EVT_DEC_CURRENT_RANGE,
    EVT_RX_CONTROL,
}; // Current max is 255 (0xFF), see cs_EventTypes.h

#ifndef TYPIFY
#define TYPIFY(NAME) NAME ## _TYPE 
#endif

typedef uint16_t TYPIFY(CONFIG_ADV_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_BOOT_DELAY);
typedef     bool TYPIFY(CONFIG_CONT_POWER_SAMPLER_ENABLED);
typedef uint16_t TYPIFY(CONFIG_CROWNSTONE_ID);
typedef    float TYPIFY(CONFIG_CURRENT_MULTIPLIER);
typedef  int32_t TYPIFY(CONFIG_CURRENT_ZERO);
typedef     bool TYPIFY(CONFIG_DEFAULT_ON);
typedef     bool TYPIFY(CONFIG_ENCRYPTION_ENABLED);
typedef  uint8_t TYPIFY(CONFIG_FLOOR);
typedef     bool TYPIFY(CONFIG_IBEACON_ENABLED);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MINOR);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MAJOR);
typedef   int8_t TYPIFY(CONFIG_IBEACON_TXPOWER); 
typedef   int8_t TYPIFY(CONFIG_LOW_TX_POWER);
typedef   int8_t TYPIFY(CONFIG_MAX_CHIP_TEMP);
typedef   int8_t TYPIFY(CONFIG_MAX_ENV_TEMP);
typedef uint32_t TYPIFY(CONFIG_MESH_ACCESS_ADDRESS); 
typedef  uint8_t TYPIFY(CONFIG_MESH_CHANNEL);
typedef     bool TYPIFY(CONFIG_MESH_ENABLED);
typedef   int8_t TYPIFY(CONFIG_MIN_ENV_TEMP);
typedef uint16_t TYPIFY(CONFIG_NEARBY_TIMEOUT);
typedef  int32_t TYPIFY(CONFIG_POWER_ZERO); 
typedef uint16_t TYPIFY(CONFIG_POWER_ZERO_AVG_WINDOW);
typedef uint32_t TYPIFY(CONFIG_PWM_PERIOD);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN);
typedef     bool TYPIFY(CONFIG_PWM_ALLOWED);
typedef uint16_t TYPIFY(CONFIG_RELAY_HIGH_DURATION);
typedef     bool TYPIFY(CONFIG_SCANNER_ENABLED);
typedef uint16_t TYPIFY(CONFIG_SCAN_BREAK_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_DURATION);
typedef  uint8_t TYPIFY(CONFIG_SCAN_FILTER);
typedef uint16_t TYPIFY(CONFIG_SCAN_FILTER_SEND_FRACTION);
typedef uint16_t TYPIFY(CONFIG_SCAN_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_SCAN_SEND_DELAY);
typedef uint16_t TYPIFY(CONFIG_SCAN_WINDOW);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM); 
typedef     bool TYPIFY(CONFIG_SWITCH_LOCKED);
typedef     bool TYPIFY(CONFIG_SWITCHCRAFT_ENABLED);
typedef    float TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD); 
typedef     bool TYPIFY(CONFIG_TRACKER_ENABLED);
typedef   int8_t TYPIFY(CONFIG_TX_POWER);
typedef  uint8_t TYPIFY(CONFIG_UART_ENABLED); 
typedef    float TYPIFY(CONFIG_VOLTAGE_MULTIPLIER);
typedef  int32_t TYPIFY(CONFIG_VOLTAGE_ZERO);

typedef  uint8_t TYPIFY(STATE_ACCUMULATED_ENERGY);
typedef  uint8_t TYPIFY(STATE_ERRORS);
typedef  uint8_t TYPIFY(STATE_ERROR_CHIP_TEMP);
typedef  uint8_t TYPIFY(STATE_ERROR_DIMMER_OFF_FAILURE);
typedef  uint8_t TYPIFY(STATE_ERROR_DIMMER_ON_FAILURE);
typedef  uint8_t TYPIFY(STATE_ERROR_OVER_CURRENT);
typedef  uint8_t TYPIFY(STATE_ERROR_OVER_CURRENT_PWM);
typedef  uint8_t TYPIFY(STATE_ERROR_PWM_TEMP);
typedef  uint8_t TYPIFY(STATE_FACTORY_RESET);
typedef  uint8_t TYPIFY(STATE_IGNORE_ALL);
typedef  uint8_t TYPIFY(STATE_IGNORE_BITMASK);
typedef  uint8_t TYPIFY(STATE_IGNORE_LOCATION);
typedef  uint8_t TYPIFY(STATE_LEARNED_SWITCHES);
typedef  uint8_t TYPIFY(STATE_OPERATION_MODE);
typedef  uint8_t TYPIFY(STATE_POWER_USAGE);
typedef  uint8_t TYPIFY(STATE_RESET_COUNTER);
typedef  uint8_t TYPIFY(STATE_SCHEDULE);
typedef  uint8_t TYPIFY(STATE_SWITCH_STATE);
typedef  uint8_t TYPIFY(STATE_TEMPERATURE);
typedef uint32_t TYPIFY(STATE_TIME);
typedef  uint8_t TYPIFY(STATE_TRACKED_DEVICES);

typedef  uint8_t TYPIFY(EVT_ADC_RESTARTED);
typedef  uint8_t TYPIFY(EVT_ADVERTISEMENT_UPDATED);
typedef  uint8_t TYPIFY(EVT_BLE_CONNECT);
typedef  uint8_t TYPIFY(EVT_BLE_DISCONNECT);
typedef  uint8_t TYPIFY(EVT_BLE_EVENT);
typedef  uint8_t TYPIFY(EVT_BROWNOUT_IMPENDING);
//typedef  uint8_t TYPIFY(EVT_CHARACTERISTICS_UPDATED);
typedef  uint8_t TYPIFY(EVT_CHIP_TEMP_ABOVE_THRESHOLD);
typedef  uint8_t TYPIFY(EVT_CHIP_TEMP_OK);
typedef  uint8_t TYPIFY(EVT_CMD_RESET);
typedef  uint8_t TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM);
typedef  uint8_t TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
typedef  uint8_t TYPIFY(EVT_DEC_CURRENT_RANGE);
typedef  uint8_t TYPIFY(EVT_DEC_VOLTAGE_RANGE);
typedef  uint8_t TYPIFY(EVT_DEVICE_SCANNED);
typedef  uint8_t TYPIFY(EVT_DIMMER_ON_FAILURE_DETECTED);
typedef  uint8_t TYPIFY(EVT_DIMMER_OFF_FAILURE_DETECTED);
typedef  uint8_t TYPIFY(EVT_DO_RESET_DELAYED);
//typedef  uint8_t TYPIFY(EVT_ENABLED_MESH);
//typedef  uint8_t TYPIFY(EVT_ENABLED_ENCRYPTION);
//typedef  uint8_t TYPIFY(EVT_ENABLED_IBEACON);
typedef  uint8_t TYPIFY(EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT);
typedef  uint8_t TYPIFY(EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE);
typedef  uint8_t TYPIFY(EVT_ENABLE_ADVERTISEMENT);
typedef  uint8_t TYPIFY(EVT_ENABLE_LOG_CURRENT);
typedef  uint8_t TYPIFY(EVT_ENABLE_LOG_FILTERED_CURRENT);
typedef  uint8_t TYPIFY(EVT_ENABLE_LOG_POWER);
typedef  uint8_t TYPIFY(EVT_ENABLE_LOG_VOLTAGE);
typedef  uint8_t TYPIFY(EVT_ENABLE_MESH);
typedef  uint8_t TYPIFY(EVT_EXTERNAL_STATE_MSG_CHAN_0);
typedef  uint8_t TYPIFY(EVT_EXTERNAL_STATE_MSG_CHAN_1);
typedef  uint8_t TYPIFY(EVT_INC_VOLTAGE_RANGE);
typedef  uint8_t TYPIFY(EVT_INC_CURRENT_RANGE);
typedef  uint8_t TYPIFY(EVT_KEEP_ALIVE);
typedef uint32_t TYPIFY(EVT_MESH_TIME);
//typedef  uint8_t TYPIFY(EVT_POWER_CONSUMPTION);
typedef  uint8_t TYPIFY(EVT_POWER_OFF);
typedef  uint8_t TYPIFY(EVT_POWER_ON);
typedef  uint8_t TYPIFY(EVT_POWER_SAMPLES_END);
typedef  uint8_t TYPIFY(EVT_POWER_SAMPLES_START);
typedef  uint8_t TYPIFY(EVT_POWER_TOGGLE);
typedef  uint8_t TYPIFY(EVT_PWM_ALLOWED);
typedef  uint8_t TYPIFY(EVT_PWM_FORCED_OFF);
typedef  uint8_t TYPIFY(EVT_PWM_POWERED);
typedef  uint8_t TYPIFY(EVT_PWM_TEMP_ABOVE_THRESHOLD);
typedef  uint8_t TYPIFY(EVT_PWM_TEMP_OK);
typedef  uint8_t TYPIFY(EVT_RELAY_FORCED_ON);
typedef  uint8_t TYPIFY(EVT_RX_CONTROL);
typedef  uint8_t TYPIFY(EVT_SCAN_STARTED);
typedef  uint8_t TYPIFY(EVT_SCAN_STOPPED);
typedef  uint8_t TYPIFY(EVT_SCANNED_DEVICES);
typedef  uint8_t TYPIFY(EVT_SCHEDULE_ENTRIES_UPDATED);
typedef  uint8_t TYPIFY(EVT_SETUP_DONE);
typedef  uint8_t TYPIFY(EVT_SET_LOG_LEVEL);
typedef  uint8_t TYPIFY(EVT_SESSION_NONCE_SET);
typedef  uint8_t TYPIFY(EVT_STATE_NOTIFICATION);
typedef  uint8_t TYPIFY(EVT_STORAGE_ERASE);
typedef  uint8_t TYPIFY(EVT_STORAGE_WRITE);
typedef  uint8_t TYPIFY(EVT_STORAGE_WRITE_DONE);
typedef     bool TYPIFY(EVT_SWITCHCRAFT_ENABLED);
typedef  uint8_t TYPIFY(EVT_SWITCH_FORCED_OFF);
typedef  uint8_t TYPIFY(EVT_SWITCH_LOCKED);
typedef  uint8_t TYPIFY(EVT_TIME_SET);
typedef  uint8_t TYPIFY(EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);
typedef  uint8_t TYPIFY(EVT_TRACKED_DEVICES);
typedef  uint8_t TYPIFY(EVT_TRACKED_DEVICE_IS_NEARBY);
typedef  uint8_t TYPIFY(EVT_TRACKED_DEVICE_NOT_NEARBY);

/**
 * Store values in FLASH or RAM. Load values from DEFAULTS, FLASH or RAM. 
 *
 * 1. Values that are written fairly often and are not important over reboots should be stored in and read from RAM.
 * For example, we can measure continuously the temperature of the chip. We can also all the time read this value 
 * from one of the BLE services. There is no reason to do a roundtrip to FLASH.
 *
 * 2. Values like CONFIG_BOOT_DELAY should be known over reboots of the device. So, it seems that they should be
 * stored in FLASH. However, with a new firmware update, these values might change. The new FIRMWARE_DEFAULT should
 * then take precedence. Henceforth, you will not be able to store items in FLASH that are set to use the
 * FIRMWARE_DEFAULT.
 *
 * 3. Values like CONFIG_IBEACON_MINOR can be configured over BLE. In other words, they are set by the user. These
 * values should persist over reboots. Although a FIRMWARE_DEFAULT might be present, this will be written to FLASH.
 * Afterwards with a new firmware update, these values also persist across the firmware update.
 */
enum class PersistenceMode: uint8_t {
    DEFAULT_PERSISTENCE, // not recommended, please, be explicit
    FLASH,
    RAM,
    FIRMWARE_DEFAULT,
    UNKNOWN
};

constexpr size16_t TypeSize(CS_TYPE const & type) {
    switch(type) {
	case CS_TYPE::CONFIG_NAME:
	    return MAX_STRING_STORAGE_SIZE+1; 
	case CS_TYPE::CONFIG_DEVICE_TYPE:
	    return 0; 
	case CS_TYPE::CONFIG_ROOM:
	    return 0; 
	case CS_TYPE::CONFIG_FLOOR:
	    return sizeof(TYPIFY(CONFIG_FLOOR));
	case CS_TYPE::CONFIG_NEARBY_TIMEOUT:
	    return sizeof(TYPIFY(CONFIG_NEARBY_TIMEOUT));
	case CS_TYPE::CONFIG_PWM_PERIOD:
	    return sizeof(TYPIFY(CONFIG_PWM_PERIOD));
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
	    return sizeof(TYPIFY(CONFIG_IBEACON_MAJOR));
	case CS_TYPE::CONFIG_IBEACON_MINOR:
	    return sizeof(TYPIFY(CONFIG_IBEACON_MINOR));
	case CS_TYPE::CONFIG_IBEACON_UUID:
	    return 16; 
	case CS_TYPE::CONFIG_IBEACON_TXPOWER:
	    return sizeof(TYPIFY(CONFIG_IBEACON_TXPOWER));
	//case CS_TYPE::CONFIG_WIFI_SETTINGS:
	//    return 0; 
	case CS_TYPE::CONFIG_TX_POWER:
	    return sizeof(TYPIFY(CONFIG_TX_POWER));
	case CS_TYPE::CONFIG_ADV_INTERVAL:
	    return sizeof(TYPIFY(CONFIG_ADV_INTERVAL));
	case CS_TYPE::CONFIG_PASSKEY:
	    return BLE_GAP_PASSKEY_LEN; 
	case CS_TYPE::CONFIG_MIN_ENV_TEMP:
	    return sizeof(TYPIFY(CONFIG_MIN_ENV_TEMP));
	case CS_TYPE::CONFIG_MAX_ENV_TEMP:
	    return sizeof(TYPIFY(CONFIG_MAX_ENV_TEMP));
	case CS_TYPE::CONFIG_SCAN_DURATION:
	    return sizeof(TYPIFY(CONFIG_SCAN_DURATION));
	case CS_TYPE::CONFIG_SCAN_SEND_DELAY:
	    return sizeof(TYPIFY(CONFIG_SCAN_SEND_DELAY));
	case CS_TYPE::CONFIG_SCAN_BREAK_DURATION:
	    return sizeof(TYPIFY(CONFIG_SCAN_BREAK_DURATION));
	case CS_TYPE::CONFIG_BOOT_DELAY:
	    return sizeof(TYPIFY(CONFIG_BOOT_DELAY));
	case CS_TYPE::CONFIG_MAX_CHIP_TEMP:
	    return sizeof(TYPIFY(CONFIG_MAX_CHIP_TEMP));
	case CS_TYPE::CONFIG_SCAN_FILTER:
	    return sizeof(TYPIFY(CONFIG_SCAN_FILTER));
	case CS_TYPE::CONFIG_SCAN_FILTER_SEND_FRACTION:
	    return sizeof(TYPIFY(CONFIG_SCAN_FILTER_SEND_FRACTION));
	//case CS_TYPE::CONFIG_CURRENT_LIMIT:
	//    return 0; 
	case CS_TYPE::CONFIG_MESH_ENABLED:
	    return sizeof(TYPIFY(CONFIG_MESH_ENABLED));
	case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
	    return sizeof(TYPIFY(CONFIG_ENCRYPTION_ENABLED));
	case CS_TYPE::CONFIG_IBEACON_ENABLED:
	    return sizeof(TYPIFY(CONFIG_IBEACON_ENABLED));
	case CS_TYPE::CONFIG_SCANNER_ENABLED:
	    return sizeof(TYPIFY(CONFIG_SCANNER_ENABLED));
	case CS_TYPE::CONFIG_CONT_POWER_SAMPLER_ENABLED:
	    return sizeof(TYPIFY(CONFIG_CONT_POWER_SAMPLER_ENABLED));
	case CS_TYPE::CONFIG_TRACKER_ENABLED:
	    return sizeof(TYPIFY(CONFIG_TRACKER_ENABLED));
	//case CS_TYPE::CONFIG_ADC_BURST_SAMPLE_RATE:
	//case CS_TYPE::CONFIG_POWER_SAMPLE_BURST_INTERVAL:
	//case CS_TYPE::CONFIG_POWER_SAMPLE_CONT_INTERVAL:
	//case CS_TYPE::CONFIG_ADC_CONT_SAMPLE_RATE:
	//    return 0; 
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
	    return sizeof(TYPIFY(CONFIG_CROWNSTONE_ID));
	case CS_TYPE::CONFIG_KEY_ADMIN:
	    return ENCRYPTION_KEY_LENGTH;
	case CS_TYPE::CONFIG_KEY_MEMBER:
	    return ENCRYPTION_KEY_LENGTH; 
	case CS_TYPE::CONFIG_KEY_GUEST:
	    return ENCRYPTION_KEY_LENGTH; 
	case CS_TYPE::CONFIG_DEFAULT_ON:
	    return sizeof(TYPIFY(CONFIG_DEFAULT_ON));
	case CS_TYPE::CONFIG_SCAN_INTERVAL:
	    return sizeof(TYPIFY(CONFIG_SCAN_INTERVAL));
	case CS_TYPE::CONFIG_SCAN_WINDOW:
	    return sizeof(TYPIFY(CONFIG_SCAN_WINDOW));
	case CS_TYPE::CONFIG_RELAY_HIGH_DURATION:
	    return sizeof(TYPIFY(CONFIG_RELAY_HIGH_DURATION));
	case CS_TYPE::CONFIG_LOW_TX_POWER:
	    return sizeof(TYPIFY(CONFIG_LOW_TX_POWER));
	case CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER:
	    return sizeof(TYPIFY(CONFIG_VOLTAGE_MULTIPLIER));
	case CS_TYPE::CONFIG_CURRENT_MULTIPLIER:
	    return sizeof(TYPIFY(CONFIG_CURRENT_MULTIPLIER));
	case CS_TYPE::CONFIG_VOLTAGE_ZERO:
	    return 0; 
	case CS_TYPE::CONFIG_CURRENT_ZERO:
	    return sizeof(TYPIFY(CONFIG_CURRENT_ZERO));
	case CS_TYPE::CONFIG_POWER_ZERO:
	    return sizeof(TYPIFY(CONFIG_POWER_ZERO));
	case CS_TYPE::CONFIG_POWER_ZERO_AVG_WINDOW:
	    return sizeof(TYPIFY(CONFIG_POWER_ZERO_AVG_WINDOW));
	case CS_TYPE::CONFIG_MESH_ACCESS_ADDRESS:
	    return 0; 
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
	    return sizeof(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD));
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM:
	    return sizeof(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM));
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
	    return sizeof(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP));
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
	    return sizeof(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN));
	case CS_TYPE::CONFIG_PWM_ALLOWED:
	    return sizeof(TYPIFY(CONFIG_PWM_ALLOWED));
	case CS_TYPE::CONFIG_SWITCH_LOCKED:
	    return sizeof(TYPIFY(CONFIG_SWITCH_LOCKED));
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
	    return sizeof(TYPIFY(CONFIG_SWITCHCRAFT_ENABLED));
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD:
	    return sizeof(TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD));
	case CS_TYPE::CONFIG_MESH_CHANNEL:
	    return sizeof(TYPIFY(CONFIG_MESH_CHANNEL));
	case CS_TYPE::CONFIG_UART_ENABLED:
	    return sizeof(TYPIFY(CONFIG_UART_ENABLED));
	case CS_TYPE::STATE_RESET_COUNTER:
	    return sizeof(TYPIFY(STATE_RESET_COUNTER));
	case CS_TYPE::STATE_SWITCH_STATE:
	    return sizeof(TYPIFY(STATE_SWITCH_STATE));
	case CS_TYPE::STATE_ACCUMULATED_ENERGY:
	    return sizeof(TYPIFY(STATE_ACCUMULATED_ENERGY));
	case CS_TYPE::STATE_POWER_USAGE:
	    return sizeof(TYPIFY(STATE_POWER_USAGE));
	case CS_TYPE::STATE_TRACKED_DEVICES:
	    return sizeof(TYPIFY(STATE_TRACKED_DEVICES));
	case CS_TYPE::STATE_SCHEDULE:
	    return sizeof(TYPIFY(STATE_SCHEDULE));
	case CS_TYPE::STATE_OPERATION_MODE:
	    return sizeof(TYPIFY(STATE_OPERATION_MODE));
	case CS_TYPE::STATE_TEMPERATURE:
	    return sizeof(TYPIFY(STATE_TEMPERATURE));
	case CS_TYPE::STATE_TIME:
	    return sizeof(TYPIFY(STATE_TIME));
	case CS_TYPE::STATE_FACTORY_RESET:
	    return sizeof(TYPIFY(STATE_FACTORY_RESET));
	case CS_TYPE::STATE_LEARNED_SWITCHES:
	    return sizeof(TYPIFY(STATE_LEARNED_SWITCHES));
	case CS_TYPE::STATE_ERRORS:
	    return sizeof(TYPIFY(STATE_ERRORS));
	case CS_TYPE::STATE_ERROR_OVER_CURRENT:
	    return sizeof(TYPIFY(STATE_ERROR_OVER_CURRENT));
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_PWM:
	    return sizeof(TYPIFY(STATE_ERROR_OVER_CURRENT_PWM));
	case CS_TYPE::STATE_ERROR_CHIP_TEMP:
	    return sizeof(TYPIFY(STATE_ERROR_CHIP_TEMP));
	case CS_TYPE::STATE_ERROR_PWM_TEMP:
	    return sizeof(TYPIFY(STATE_ERROR_PWM_TEMP));
	case CS_TYPE::STATE_IGNORE_BITMASK:
	    return sizeof(TYPIFY(STATE_IGNORE_BITMASK));
	case CS_TYPE::STATE_IGNORE_ALL:
	    return sizeof(TYPIFY(STATE_IGNORE_ALL));
	case CS_TYPE::STATE_IGNORE_LOCATION:
	    return sizeof(TYPIFY(STATE_IGNORE_LOCATION));
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE:
	    return sizeof(TYPIFY(STATE_ERROR_DIMMER_ON_FAILURE));
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE:
	    return sizeof(TYPIFY(STATE_ERROR_DIMMER_OFF_FAILURE));
	case CS_TYPE::EVT_POWER_OFF:
	    return sizeof(TYPIFY(EVT_POWER_OFF));
	case CS_TYPE::EVT_POWER_ON:
	    return sizeof(TYPIFY(EVT_POWER_ON));
	case CS_TYPE::EVT_POWER_TOGGLE:
	    return sizeof(TYPIFY(EVT_POWER_TOGGLE));
	case CS_TYPE::EVT_ADVERTISEMENT_UPDATED:
	    return sizeof(TYPIFY(EVT_ADVERTISEMENT_UPDATED));
	case CS_TYPE::EVT_SCAN_STARTED:
	    return sizeof(TYPIFY(EVT_SCAN_STARTED));
	case CS_TYPE::EVT_SCAN_STOPPED:
	    return sizeof(TYPIFY(EVT_SCAN_STOPPED));
	case CS_TYPE::EVT_SCANNED_DEVICES:
	    return sizeof(TYPIFY(EVT_SCANNED_DEVICES));
	case CS_TYPE::EVT_DEVICE_SCANNED:
	    return sizeof(TYPIFY(EVT_DEVICE_SCANNED));
	case CS_TYPE::EVT_POWER_SAMPLES_START:
	    return sizeof(TYPIFY(EVT_POWER_SAMPLES_START));
	case CS_TYPE::EVT_POWER_SAMPLES_END:
	    return sizeof(TYPIFY(EVT_POWER_SAMPLES_END));
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM:
	    return sizeof(TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM));
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
	    return sizeof(TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD));
	case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
	    return sizeof(TYPIFY(EVT_DIMMER_ON_FAILURE_DETECTED));
	case CS_TYPE::EVT_DIMMER_OFF_FAILURE_DETECTED:
	    return sizeof(TYPIFY(EVT_DIMMER_OFF_FAILURE_DETECTED));
	//case CS_TYPE::EVT_POWER_CONSUMPTION:
	//    return sizeof(TYPIFY(EVT_POWER_CONSUMPTION));
	//case CS_TYPE::EVT_ENABLED_MESH:
	//    return sizeof(TYPIFY(EVT_ENABLED_MESH));
	//case CS_TYPE::EVT_ENABLED_ENCRYPTION:
	//    return sizeof(TYPIFY(EVT_ENABLED_ENCRYPTION));
	//case CS_TYPE::EVT_ENABLED_IBEACON:
	//    return sizeof(TYPIFY(EVT_ENABLED_IBEACON));
	//case CS_TYPE::EVT_CHARACTERISTICS_UPDATED:
	//    return sizeof(TYPIFY(EVT_CHARACTERISTICS_UPDATED));
	case CS_TYPE::EVT_TRACKED_DEVICES:
	    return sizeof(TYPIFY(EVT_TRACKED_DEVICES));
	case CS_TYPE::EVT_TRACKED_DEVICE_IS_NEARBY:
	    return sizeof(TYPIFY(EVT_TRACKED_DEVICE_IS_NEARBY));
	case CS_TYPE::EVT_TRACKED_DEVICE_NOT_NEARBY:
	    return sizeof(TYPIFY(EVT_TRACKED_DEVICE_NOT_NEARBY));
	case CS_TYPE::EVT_MESH_TIME:
	    return sizeof(TYPIFY(EVT_MESH_TIME));
	case CS_TYPE::EVT_SCHEDULE_ENTRIES_UPDATED:
	    return sizeof(TYPIFY(EVT_SCHEDULE_ENTRIES_UPDATED));
	case CS_TYPE::EVT_BLE_EVENT:
	    return sizeof(TYPIFY(EVT_BLE_EVENT));
	case CS_TYPE::EVT_BLE_CONNECT:
	    return sizeof(TYPIFY(EVT_BLE_CONNECT));
	case CS_TYPE::EVT_BLE_DISCONNECT:
	    return sizeof(TYPIFY(EVT_BLE_DISCONNECT));
	case CS_TYPE::EVT_STATE_NOTIFICATION:
	    return sizeof(TYPIFY(EVT_STATE_NOTIFICATION));
	case CS_TYPE::EVT_BROWNOUT_IMPENDING:
	    return sizeof(TYPIFY(EVT_BROWNOUT_IMPENDING));
	case CS_TYPE::EVT_SESSION_NONCE_SET:
	    return sizeof(TYPIFY(EVT_SESSION_NONCE_SET));
	case CS_TYPE::EVT_KEEP_ALIVE:
	    return sizeof(TYPIFY(EVT_KEEP_ALIVE));
	case CS_TYPE::EVT_PWM_FORCED_OFF:
	    return sizeof(TYPIFY(EVT_PWM_FORCED_OFF));
	case CS_TYPE::EVT_SWITCH_FORCED_OFF:
	    return sizeof(TYPIFY(EVT_SWITCH_FORCED_OFF));
	case CS_TYPE::EVT_RELAY_FORCED_ON:
	    return sizeof(TYPIFY(EVT_RELAY_FORCED_ON));
	case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
	    return sizeof(TYPIFY(EVT_CHIP_TEMP_ABOVE_THRESHOLD));
	case CS_TYPE::EVT_CHIP_TEMP_OK:
	    return sizeof(TYPIFY(EVT_CHIP_TEMP_OK));
	case CS_TYPE::EVT_PWM_TEMP_ABOVE_THRESHOLD:
	    return sizeof(TYPIFY(EVT_PWM_TEMP_ABOVE_THRESHOLD));
	case CS_TYPE::EVT_PWM_TEMP_OK:
	    return sizeof(TYPIFY(EVT_PWM_TEMP_OK));
	case CS_TYPE::EVT_EXTERNAL_STATE_MSG_CHAN_0:
	    return sizeof(TYPIFY(EVT_EXTERNAL_STATE_MSG_CHAN_0));
	case CS_TYPE::EVT_EXTERNAL_STATE_MSG_CHAN_1:
	    return sizeof(TYPIFY(EVT_EXTERNAL_STATE_MSG_CHAN_1));
	case CS_TYPE::EVT_TIME_SET:
	    return sizeof(TYPIFY(EVT_TIME_SET));
	case CS_TYPE::EVT_PWM_POWERED:
	    return sizeof(TYPIFY(EVT_PWM_POWERED));
	case CS_TYPE::EVT_PWM_ALLOWED:
	    return sizeof(TYPIFY(EVT_PWM_ALLOWED));
	case CS_TYPE::EVT_SWITCH_LOCKED:
	    return sizeof(TYPIFY(EVT_SWITCH_LOCKED));
	case CS_TYPE::EVT_STORAGE_WRITE_DONE:
	    return sizeof(TYPIFY(EVT_STORAGE_WRITE_DONE));
	case CS_TYPE::EVT_SETUP_DONE:
	    return sizeof(TYPIFY(EVT_SETUP_DONE));
	case CS_TYPE::EVT_DO_RESET_DELAYED:
	    return sizeof(TYPIFY(EVT_DO_RESET_DELAYED));
	case CS_TYPE::EVT_SWITCHCRAFT_ENABLED:
	    return sizeof(TYPIFY(EVT_SWITCHCRAFT_ENABLED));
	case CS_TYPE::EVT_STORAGE_WRITE:
	    return sizeof(TYPIFY(EVT_STORAGE_WRITE));
	case CS_TYPE::EVT_STORAGE_ERASE:
	    return sizeof(TYPIFY(EVT_STORAGE_ERASE));
	case CS_TYPE::EVT_ADC_RESTARTED:
	    return sizeof(TYPIFY(EVT_ADC_RESTARTED));
	case CS_TYPE::EVT_SET_LOG_LEVEL:
	    return sizeof(TYPIFY(EVT_SET_LOG_LEVEL));
	case CS_TYPE::EVT_ENABLE_LOG_POWER:
	    return sizeof(TYPIFY(EVT_ENABLE_LOG_POWER));
	case CS_TYPE::EVT_ENABLE_LOG_CURRENT:
	    return sizeof(TYPIFY(EVT_ENABLE_LOG_CURRENT));
	case CS_TYPE::EVT_ENABLE_LOG_VOLTAGE:
	    return sizeof(TYPIFY(EVT_ENABLE_LOG_VOLTAGE));
	case CS_TYPE::EVT_ENABLE_LOG_FILTERED_CURRENT:
	    return sizeof(TYPIFY(EVT_ENABLE_LOG_FILTERED_CURRENT));
	case CS_TYPE::EVT_CMD_RESET:
	    return sizeof(TYPIFY(EVT_CMD_RESET));
	case CS_TYPE::EVT_ENABLE_ADVERTISEMENT:
	    return sizeof(TYPIFY(EVT_ENABLE_ADVERTISEMENT));
	case CS_TYPE::EVT_ENABLE_MESH:
	    return sizeof(TYPIFY(EVT_ENABLE_MESH));
	case CS_TYPE::EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN:
	    return sizeof(TYPIFY(EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN));
	case CS_TYPE::EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT:
	    return sizeof(TYPIFY(EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT));
	case CS_TYPE::EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE:
	    return sizeof(TYPIFY(EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE));
	case CS_TYPE::EVT_INC_VOLTAGE_RANGE:
	    return sizeof(TYPIFY(EVT_INC_VOLTAGE_RANGE));
	case CS_TYPE::EVT_DEC_VOLTAGE_RANGE:
	    return sizeof(TYPIFY(EVT_DEC_VOLTAGE_RANGE));
	case CS_TYPE::EVT_INC_CURRENT_RANGE:
	    return sizeof(TYPIFY(EVT_INC_CURRENT_RANGE));
	case CS_TYPE::EVT_DEC_CURRENT_RANGE:
	    return sizeof(TYPIFY(EVT_DEC_CURRENT_RANGE));
	case CS_TYPE::EVT_RX_CONTROL:
	    return sizeof(TYPIFY(EVT_RX_CONTROL));
    }
    // should never happen
    return 0;
}

constexpr const char* TypeName(CS_TYPE const & type) {
    switch(type) {
	case CS_TYPE::STATE_RESET_COUNTER:
	    return "STATE_RESET_COUNTER";
	case CS_TYPE::STATE_SWITCH_STATE:
	    return "STATE_SWITCH_STATE";
	case CS_TYPE::STATE_ACCUMULATED_ENERGY:
	    return "STATE_ACCUMULATED_ENERGY";
	case CS_TYPE::STATE_POWER_USAGE:
	    return "STATE_POWER_USAGE";
	case CS_TYPE::STATE_TRACKED_DEVICES:
	    return "STATE_TRACKED_DEVICES";
	case CS_TYPE::STATE_SCHEDULE:
	    return "STATE_SCHEDULE";
	case CS_TYPE::STATE_OPERATION_MODE:
	    return "STATE_OPERATION_MODE";
	case CS_TYPE::STATE_TEMPERATURE:
	    return "STATE_TEMPERATURE";
	case CS_TYPE::STATE_TIME:
	    return "STATE_TIME";
	case CS_TYPE::STATE_FACTORY_RESET:
	    return "STATE_FACTORY_RESET";
	case CS_TYPE::STATE_LEARNED_SWITCHES:
	    return "STATE_LEARNED_SWITCHES";
	case CS_TYPE::STATE_ERRORS:
	    return "STATE_ERRORS";
	case CS_TYPE::STATE_ERROR_OVER_CURRENT:
	    return "STATE_ERROR_OVER_CURRENT";
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_PWM:
	    return "STATE_ERROR_OVER_CURRENT_PWM";
	case CS_TYPE::STATE_ERROR_CHIP_TEMP:
	    return "STATE_ERROR_CHIP_TEMP";
	case CS_TYPE::STATE_ERROR_PWM_TEMP:
	    return "STATE_ERROR_PWM_TEMP";
	case CS_TYPE::STATE_IGNORE_BITMASK:
	    return "STATE_IGNORE_BITMASK";
	case CS_TYPE::STATE_IGNORE_ALL:
	    return "STATE_IGNORE_ALL";
	case CS_TYPE::STATE_IGNORE_LOCATION:
	    return "STATE_IGNORE_LOCATION";
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE:
	    return "STATE_ERROR_DIMMER_ON_FAILURE";
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE:
	    return "STATE_ERROR_DIMMER_OFF_FAILURE";
	default:
	    return "Unknown";
    }
}

constexpr PersistenceMode DefaultPersistence(CS_TYPE const & type) {
    switch(type) {
	case CS_TYPE::CONFIG_NAME:
	case CS_TYPE::CONFIG_DEVICE_TYPE:
	case CS_TYPE::CONFIG_ROOM:
	case CS_TYPE::CONFIG_FLOOR:
	case CS_TYPE::CONFIG_NEARBY_TIMEOUT:
	case CS_TYPE::CONFIG_PWM_PERIOD:
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
	case CS_TYPE::CONFIG_IBEACON_MINOR:
	case CS_TYPE::CONFIG_IBEACON_UUID:
	case CS_TYPE::CONFIG_IBEACON_TXPOWER:
	//case CS_TYPE::CONFIG_WIFI_SETTINGS:
	case CS_TYPE::CONFIG_TX_POWER:
	case CS_TYPE::CONFIG_ADV_INTERVAL:
	case CS_TYPE::CONFIG_PASSKEY:
	case CS_TYPE::CONFIG_MIN_ENV_TEMP:
	case CS_TYPE::CONFIG_MAX_ENV_TEMP:
	case CS_TYPE::CONFIG_SCAN_DURATION:
	case CS_TYPE::CONFIG_SCAN_SEND_DELAY:
	case CS_TYPE::CONFIG_SCAN_BREAK_DURATION:
	case CS_TYPE::CONFIG_BOOT_DELAY:
	case CS_TYPE::CONFIG_MAX_CHIP_TEMP:
	case CS_TYPE::CONFIG_SCAN_FILTER:
	case CS_TYPE::CONFIG_SCAN_FILTER_SEND_FRACTION:
	//case CS_TYPE::CONFIG_CURRENT_LIMIT:
	case CS_TYPE::CONFIG_MESH_ENABLED:
	case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
	case CS_TYPE::CONFIG_IBEACON_ENABLED:
	case CS_TYPE::CONFIG_SCANNER_ENABLED:
	case CS_TYPE::CONFIG_CONT_POWER_SAMPLER_ENABLED:
	case CS_TYPE::CONFIG_TRACKER_ENABLED:
	//case CS_TYPE::CONFIG_ADC_BURST_SAMPLE_RATE:
	//case CS_TYPE::CONFIG_POWER_SAMPLE_BURST_INTERVAL:
	//case CS_TYPE::CONFIG_POWER_SAMPLE_CONT_INTERVAL:
	//case CS_TYPE::CONFIG_ADC_CONT_SAMPLE_RATE:
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
	case CS_TYPE::CONFIG_KEY_ADMIN:
	case CS_TYPE::CONFIG_KEY_MEMBER:
	case CS_TYPE::CONFIG_KEY_GUEST:
	case CS_TYPE::CONFIG_DEFAULT_ON:
	case CS_TYPE::CONFIG_SCAN_INTERVAL:
	case CS_TYPE::CONFIG_SCAN_WINDOW:
	case CS_TYPE::CONFIG_RELAY_HIGH_DURATION:
	case CS_TYPE::CONFIG_LOW_TX_POWER:
	case CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER:
	case CS_TYPE::CONFIG_CURRENT_MULTIPLIER:
	case CS_TYPE::CONFIG_VOLTAGE_ZERO:
	case CS_TYPE::CONFIG_CURRENT_ZERO:
	case CS_TYPE::CONFIG_POWER_ZERO:
	case CS_TYPE::CONFIG_POWER_ZERO_AVG_WINDOW:
	case CS_TYPE::CONFIG_MESH_ACCESS_ADDRESS:
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM:
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
	case CS_TYPE::CONFIG_PWM_ALLOWED:
	case CS_TYPE::CONFIG_SWITCH_LOCKED:
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD:
	case CS_TYPE::CONFIG_MESH_CHANNEL:
	case CS_TYPE::CONFIG_UART_ENABLED:
	    return PersistenceMode::FLASH;
	case CS_TYPE::STATE_RESET_COUNTER:
	case CS_TYPE::STATE_SWITCH_STATE:
	case CS_TYPE::STATE_ACCUMULATED_ENERGY:
	case CS_TYPE::STATE_POWER_USAGE:
	case CS_TYPE::STATE_TRACKED_DEVICES:
	case CS_TYPE::STATE_SCHEDULE:
	case CS_TYPE::STATE_OPERATION_MODE:
	case CS_TYPE::STATE_TEMPERATURE:
	case CS_TYPE::STATE_TIME:
	case CS_TYPE::STATE_FACTORY_RESET:
	case CS_TYPE::STATE_LEARNED_SWITCHES:
	case CS_TYPE::STATE_ERRORS:
	case CS_TYPE::STATE_ERROR_OVER_CURRENT:
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_PWM:
	case CS_TYPE::STATE_ERROR_CHIP_TEMP:
	case CS_TYPE::STATE_ERROR_PWM_TEMP:
	case CS_TYPE::STATE_IGNORE_BITMASK:
	case CS_TYPE::STATE_IGNORE_ALL:
	case CS_TYPE::STATE_IGNORE_LOCATION:
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE:
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE:
	    return PersistenceMode::RAM;
	default:
	    return PersistenceMode::UNKNOWN;
    }
}

/**
 * Custom structs
 */

struct state_vars_notifaction {
	uint8_t type;
	uint8_t* data;
	uint16_t dataLength;
};

union state_errors_t {
	struct __attribute__((packed)) {
		uint8_t overCurrent : 1;
		uint8_t overCurrentPwm : 1;
		uint8_t chipTemp : 1;
		uint8_t pwmTemp : 1;
		uint8_t dimmerOn : 1;
		uint8_t dimmerOff : 1;
		uint32_t reserved : 26;
	} errors;
	uint32_t asInt;
};

union state_ignore_bitmask_t {
	struct __attribute__((packed)) {
		uint8_t all : 1;
		uint8_t location : 1;
		uint8_t reserved : 6;
	} mask;
	uint8_t asInt;
};
