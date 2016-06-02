/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

/** Service UUIDs
 *
 * Note: The last 4 digits of the first tuple should be 0. They will be used
 *       by the characteristic to set its UUID.
 *
 * E.g.: General Service UUID            -> f5f90000-59f9-11e4-aa15-123b93f75cba
 *       Temperature Characteristic UUID -> f5f90000-59f9-11e4-aa15-123b93f75cba ||
 *       								        0001
 *       								 -> f5f90001-59f9-11e4-aa15-123b93f75cba
 */

#define POWER_UUID                          "5b8d0000-6f20-11e4-b116-123b93f75cba"
#define GENERAL_UUID                        "f5f90000-59f9-11e4-aa15-123b93f75cba"
#define INDOORLOCALISATION_UUID             "7e170000-429c-41aa-83d7-d91220abeb33"
#define ALERT_UUID                          "33690000-2a0a-11e5-b345-feff819cdc9f"
#define SCHEDULE_UUID                       "96d20000-4bcf-11e5-885d-feff819cdc9f"
#define DEVICE_INFORMATION_UUID             "180a"
#define CROWNSTONE_UUID                     "5432"
#define SETUP_UUID							"9b4c0000-983b-46d9-ae89-f9dedddcd15c"

/** Characteristic UUIDs */

enum PowerCharacteristicsIDs {
	PWM_UUID                                = 0x1, //cmd
	SAMPLE_CURRENT_UUID                     = 0x2, // remove?
	CURRENT_CURVE_UUID                      = 0x3, //state
	CURRENT_CONSUMPTION_UUID                = 0x4, //state
//	CURRENT_LIMIT_UUID                      = 0x5, //cfg

	RELAY_UUID                              = 0x6, //cmd
};

enum SetupCharacteristicsIDs {
	MAC_ADDRESS_UUID                        = 0x2
};

enum GeneralCharacteristicsIDs {
	CONTROL_UUID                            = 0x1,
	MESH_CONTROL_UUID                       = 0x2,
	WRITE_CONFIGURATION_UUID                = 0x7, // 0x03
	READ_CONFIGURATION_UUID                 = 0x9, // 0x04
	WRITE_STATEVAR_UUID                     = 0xA, // 0x05
	READ_STATEVAR_UUID                      = 0xB, // 0x06
	TEMPERATURE_UUID                        = 0xC, // 0x07
	RESET_UUID                              = 0x5, // 0x08
};

enum IndoorLocalizationCharacteristicsIDs {
	RSSI_UUID                               = 0x1, //state
	TRACKED_DEVICE_UUID                     = 0x2, //behav
	SCAN_DEVICE_UUID                        = 0x3, // remove?
	LIST_DEVICE_UUID                        = 0x4, // remove?
	TRACKED_DEVICE_LIST_UUID                = 0x5, //behav
};//behav

enum AlertCharacteristicsIDs {
	SUPPORTED_NEW_ALERT_UUID                = 0x1,
	NEW_ALERT_UUID                          = 0x2,
	SUPPORTED_UNREAD_ALERT_UUID             = 0x3,
	UNREAD_ALERT_UUID                       = 0x4,
	ALERT_CONTROL_POINT_UUID                = 0x5,
};

enum ScheduleCharacteristicsIDs {
	CURRENT_TIME_UUID                       = 0x1, //state
	WRITE_SCHEDULE_ENTRY_UUID               = 0x2, //behav
	LIST_SCHEDULE_ENTRIES_UUID              = 0x3, //behav
};
