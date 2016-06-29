/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jun 8, 2016
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventTypes.h>

/** Configuration types
 *
 * Use in the characteristic to read and write configurations in <CommonService>.
 */
enum ConfigurationTypes {
	CONFIG_NAME                             = Configuration_Base,
	CONFIG_DEVICE_TYPE                      = 1,      //! 0x01
	CONFIG_ROOM                             = 2,      //! 0x02
	CONFIG_FLOOR                            = 3,      //! 0x03
	CONFIG_NEARBY_TIMEOUT                   = 4,      //! 0x04
	CONFIG_PWM_FREQ                         = 5,      //! 0x05
	CONFIG_IBEACON_MAJOR                    = 6,      //! 0x06
	CONFIG_IBEACON_MINOR                    = 7,      //! 0x07
	CONFIG_IBEACON_UUID                     = 8,      //! 0x08
	CONFIG_IBEACON_TXPOWER                  = 9,      //! 0x09
	CONFIG_WIFI_SETTINGS                    = 10,     //! 0x0A
	CONFIG_TX_POWER                         = 11,     //! 0x0B
	CONFIG_ADV_INTERVAL                     = 12,     //! 0x0C
	CONFIG_PASSKEY							= 13,     //! 0x0D
	CONFIG_MIN_ENV_TEMP                     = 14,     //! 0x0E
	CONFIG_MAX_ENV_TEMP                     = 15,     //! 0x0F
	CONFIG_SCAN_DURATION                    = 16,     //! 0x10
	CONFIG_SCAN_SEND_DELAY                  = 17,     //! 0x11
	CONFIG_SCAN_BREAK_DURATION              = 18,     //! 0x12
	CONFIG_BOOT_DELAY                       = 19,     //! 0x13
	CONFIG_MAX_CHIP_TEMP                    = 20,     //! 0x14
	CONFIG_SCAN_FILTER                      = 21,     //! 0x15
	CONFIG_SCAN_FILTER_SEND_FRACTION        = 22,     //! 0x16
	CONFIG_CURRENT_LIMIT                    = 23,     //! 0x17
	CONFIG_MESH_ENABLED                     = 24,     //! 0x18
	CONFIG_ENCRYPTION_ENABLED               = 25,     //! 0x19
	CONFIG_IBEACON_ENABLED                  = 26,     //! 0x1A
	CONFIG_SCANNER_ENABLED                  = 27,     //! 0x1B
	CONFIG_CONT_POWER_SAMPLER_ENABLED       = 28,     //! 0x1C
	CONFIG_TRACKER_ENABLED                  = 29,     //! 0x1D
	CONFIG_ADC_BURST_SAMPLE_RATE            = 30,     //! 0x1E
	CONFIG_POWER_SAMPLE_BURST_INTERVAL      = 31,     //! 0x1F
	CONFIG_POWER_SAMPLE_CONT_INTERVAL       = 32,     //! 0x20
	CONFIG_ADC_CONT_SAMPLE_RATE             = 33,     //! 0x1E
	CONFIG_CROWNSTONE_ID                    = 34,     //! 0x22
	CONFIG_KEY_OWNER                        = 35,     //! 0x23
	CONFIG_KEY_MEMBER                       = 36,     //! 0x24
	CONFIG_KEY_GUEST                        = 37,     //! 0x25
	CONFIG_DEFAULT_ON                       = 38,
	CONFIG_SCAN_INTERVAL                    = 39,
	CONFIG_SCAN_WINDOW                      = 40,
	CONFIG_RELAY_HIGH_DURATION              = 41,
	CONFIG_LOW_TX_POWER                     = 42,
	CONFIG_VOLTAGE_MULTIPLIER               = 43,
	CONFIG_CURRENT_MULTIPLIER               = 44,
	CONFIG_VOLTAGE_ZERO					    = 45,
	CONFIG_CURRENT_ZERO                     = 46,
	CONFIG_POWER_ZERO                       = 47,

	CONFIG_TYPES
};
