/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

/** Service UUIDs
 *
 * We are using an incremental UUID pattern with base UUID
 *    24f00000-7d10-4805-bfc1-7663a01c3bff
 *
 * We increment the 4th bit of the first tuple for every service:
 *    Crownstone Service : 0
 *    Setup Service      : 1
 *    General Service    : 2
 *    Power Service      : 3
 *    Indoor Service     : 4
 *    Schedule Service   : 5
 *    Alert Service      : 6
 *
 * The last 4 digits of the first tuple should be 0. They are used by the
 * characteristics to set their UUIDs.
 *
 * E.g. the Setup Service has UUID
 *   24f10000-7d10-4805-bfc1-7663a01c3bff
 * and the MAC Address Characteristic of the Setup Service has UUID
 *   24f10002-7d10-4805-bfc1-7663a01c3bff
 *
 */

/**
 * Custom Service UUIDs
 */
//                                 byte nr:  15141312 1110  9 8  7 6  5 4 3 2 1 0
#define CROWNSTONE_UUID                     "24f00000-7d10-4805-bfc1-7663a01c3bff"
#define SETUP_UUID                          "24f10000-7d10-4805-bfc1-7663a01c3bff"
#define GENERAL_UUID                        "24f20000-7d10-4805-bfc1-7663a01c3bff"
#define POWER_UUID                          "24f30000-7d10-4805-bfc1-7663a01c3bff"
#define INDOORLOCALISATION_UUID             "24f40000-7d10-4805-bfc1-7663a01c3bff"
#define SCHEDULE_UUID                       "24f50000-7d10-4805-bfc1-7663a01c3bff"
#define ALERT_UUID                          "24f60000-7d10-4805-bfc1-7663a01c3bff"

//! UUID used for the Service Data in the Scan Response packet
#define CROWNSTONE_PLUG_SERVICE_DATA_UUID   0xC001
#define CROWNSTONE_BUILT_SERVICE_DATA_UUID  0xC002 // Deprecated
#define GUIDESTONE_SERVICE_DATA_UUID        0xC003 // Deprecated

enum CrownstoneCharacteristicsIDs {
	CONTROL_UUID                            = 0x1,
	MESH_CONTROL_UUID                       = 0x2,
//	MESH_READ_UUID                          = 0x3,
	CONFIG_CONTROL_UUID                     = 0x4,
	CONFIG_READ_UUID                        = 0x5,
	STATE_CONTROL_UUID                      = 0x6,
	STATE_READ_UUID                         = 0x7,
	SESSION_NONCE_UUID                      = 0x8,
	FACTORY_RESET_UUID                      = 0x9,
};

enum SetupCharacteristicsIDs {
//	SETUP_CONTROL_UUID                      = 0x1, // Changed to 7, because old setup command is deprecated.
	MAC_ADDRESS_UUID                        = 0x2,
	SETUP_KEY_UUID                          = 0x3,
//	CONFIG_CONTROL_UUID                     = 0x4, // is taken from CrownstonecharacteristicIDs, mentioned here only for completeness' sake
//	CONFIG_READ_UUID                        = 0x5, // is taken from CrownstonecharacteristicIDs, mentioned here only for completeness' sake
	GOTO_DFU_UUID                           = 0x6,
	SETUP_CONTROL_UUID                      = 0x7,
//	SESSION_NONCE_UUID                      = 0x8, // is taken from CrownstonecharacteristicIDs, mentioned here only for completeness' sake
};

enum GeneralCharacteristicsIDs {
	TEMPERATURE_UUID                        = 0x1,
	RESET_UUID                              = 0x2,
};

/** Characteristic UUIDs */
enum PowerCharacteristicsIDs {
	PWM_UUID                                = 0x1, //cmd
	RELAY_UUID                              = 0x2, //cmd
	POWER_SAMPLES_UUID                      = 0x3, //state
	POWER_CONSUMPTION_UUID                  = 0x4, //state
};

enum IndoorLocalizationCharacteristicsIDs {
	TRACKED_DEVICE_UUID                     = 0x1, //behave, cmd?
	TRACKED_DEVICE_LIST_UUID                = 0x2, //state
	SCAN_CONTROL_UUID                       = 0x3, //cmd
	LIST_DEVICE_UUID                        = 0x4, //state
	RSSI_UUID                               = 0x5, //state
};

enum ScheduleCharacteristicsIDs {
	CURRENT_TIME_UUID                       = 0x1, //state
	WRITE_SCHEDULE_ENTRY_UUID               = 0x2, //behav
	LIST_SCHEDULE_ENTRIES_UUID              = 0x3, //behav
};

enum AlertCharacteristicsIDs {
	SUPPORTED_NEW_ALERT_UUID                = 0x1,
	NEW_ALERT_UUID                          = 0x2,
	SUPPORTED_UNREAD_ALERT_UUID             = 0x3,
	UNREAD_ALERT_UUID                       = 0x4,
	ALERT_CONTROL_POINT_UUID                = 0x5,
};
