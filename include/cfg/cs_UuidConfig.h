/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

/* Service UUIDs
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

/* Characteristic UUIDs */

enum PowerCharacteristicsIDs {
	PWM_UUID                                = 0x1,
	SAMPLE_CURRENT_UUID                     = 0x2,
	CURRENT_CURVE_UUID                      = 0x3,
	CURRENT_CONSUMPTION_UUID                = 0x4,
	CURRENT_LIMIT_UUID                      = 0x5,
};

enum GeneralCharacteristicsIDs {
	TEMPERATURE_UUID                        = 0x1,
	CHANGE_NAME_UUID                        = 0x2,
	DEVICE_TYPE_UUID                        = 0x3,
	ROOM_UUID                               = 0x4,
	RESET_UUID                              = 0x5,
	MESH_UUID                               = 0x6,
	SET_CONFIGURATION_UUID                  = 0x7,
	SELECT_CONFIGURATION_UUID               = 0x8,
	GET_CONFIGURATION_UUID                  = 0x9,
};

enum IndoorLocalizationCharacteristicsIDs {
	RSSI_UUID                               = 0x1,
	TRACKED_DEVICE_UUID                     = 0x2,
	SCAN_DEVICE_UUID                        = 0x3,
	LIST_DEVICE_UUID                        = 0x4,
	TRACKED_DEVICE_LIST_UUID                = 0x5,
};
