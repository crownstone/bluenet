/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

/** Service UUIDs
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

//! UUID used for the Service Data in the Scan Response packet
#define CROWNSTONE_PLUG_SERVICE_DATA_UUID   0xC001
#define CROWNSTONE_BUILT_SERVICE_DATA_UUID  0xC002 // Deprecated
#define GUIDESTONE_SERVICE_DATA_UUID        0xC003 // Deprecated

enum CrownstoneCharacteristicsIDs {
//	CONTROL_UUID                            = 0x1, // Changed to A, because we the header is different, and we added the result characteristic.
//	MESH_CONTROL_UUID                       = 0x2, // Removed.
//	MESH_READ_UUID                          = 0x3,
//	CONFIG_CONTROL_UUID                     = 0x4, // Removed.
//	CONFIG_READ_UUID                        = 0x5, // Removed.
//	STATE_CONTROL_UUID                      = 0x6, // Removed.
//	STATE_READ_UUID                         = 0x7, // Removed.
//	SESSION_NONCE_UUID                      = 0x8, // Changed to E, as we added protocol version.
	FACTORY_RESET_UUID                      = 0x9,
//	CONTROL_UUID                            = 0xA, // Changed to C, as State get and set changed.
//	RESULT_UUID                             = 0xB, // Changed to D, as State get and set changed.
	CONTROL_UUID                            = 0xC,
	RESULT_UUID                             = 0xD,
	SESSION_DATA_UUID                       = 0xE,
	SESSION_DATA_UNENCRYPTED_UUID           = 0xF,
};

enum SetupCharacteristicsIDs {
//	SETUP_CONTROL_UUID                      = 0x1, // Changed to 7, because old setup command is deprecated.
	MAC_ADDRESS_UUID                        = 0x2,
	SETUP_KEY_UUID                          = 0x3,
//	CONFIG_CONTROL_UUID                     = 0x4, // Removed.
//	CONFIG_READ_UUID                        = 0x5, // Removed.
//	GOTO_DFU_UUID                           = 0x6, // Removed, use control command instead.
//	SETUP_CONTROL_UUID                      = 0x7, // Changed to 9, because we added more keys to setup.
//	SESSION_NONCE_UUID                      = 0x8, // Changed to E, as we added protocol version.
//	SETUP_CONTROL_UUID                      = 0x9, // Changed to A, because we the header is different, and we added the result characteristic.
//	SETUP_CONTROL_UUID                      = 0xA, // Changed to C, as State get and set changed.
//	SETUP_RESULT_UUID                       = 0xB, // Changed to D, as State get and set changed.
	SETUP_CONTROL_UUID                      = 0xC,
	SETUP_RESULT_UUID                       = 0xD,
//	SESSION_DATA_UUID                       = 0xE, // is taken from CrownstonecharacteristicIDs, mentioned here only for completeness' sake
//	SESSION_DATA_UNENCRYPTED_UUID           = 0xF, // is taken from CrownstonecharacteristicIDs, mentioned here only for completeness' sake
};

