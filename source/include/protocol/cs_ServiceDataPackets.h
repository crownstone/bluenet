/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 22, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <protocol/cs_Typedefs.h>

#define SERVICE_DATA_VALIDATION 0xFA



union __attribute__((packed)) service_data_hub_state_flags_t {
	struct __attribute__((packed)) {
		bool uartAlive : 1;                      // Whether the UART connection is alive (heartbeats are received).
		bool uartAliveEncrypted : 1;             // Whether the UART connection is alive (encrypted heartbeats are received).
		bool uartEncryptionRequiredByStone : 1;  // Whether the encrypted UART is required by this Crownstone.
		bool uartEncryptionRequiredByHub : 1;    // Whether the encrypted UART is required by the hub.
		bool hasBeenSetUp : 1;                   // Whether the hub has been set up.
		bool hasInternet : 1;                    // Whether the hub has internet connection.
		bool hasError : 1;                       // Whether the hub has some error.
	} flags;
	uint8_t asInt;
};

#define SERVICE_DATA_HUB_DATA_SIZE 9

struct __attribute__((packed)) service_data_hub_state_t {
	uint8_t  id;
	service_data_hub_state_flags_t flags;
	uint8_t  hubData[SERVICE_DATA_HUB_DATA_SIZE];
	uint16_t partialTimestamp;
	uint8_t  reserved; // Only required if we want to send hub state over mesh.
	uint8_t  validation;
};



union __attribute__((packed)) service_data_state_flags_t {
	struct __attribute__((packed)) {
		bool dimmingReady : 1;         // Whether the dimmer is ready to be used.
		bool markedDimmable : 1;       // Whether dimming is configured to be allowed
		bool error : 1;                // Whether the Crownstone has an error.
		bool switchLocked : 1;         // Whether the switch is locked.
		bool timeSet : 1;              // Whether the time is set.
		bool switchcraft : 1;          // Whether switchcraft is enabled.
		bool tapToToggle : 1;          // Whether tap to toggle is enabled.
		bool behaviourOverridden : 1;  // Whether behaviour is overridden.
	} flags;
	uint8_t asInt;
};

union __attribute__((packed)) service_data_state_extra_flags_t {
	struct __attribute__((packed)) {
		bool behaviourEnabled : 1;     // Whether behaviour is enabled.
	} flags;
	uint8_t asInt;
};

/**
 * State of this crownstone.
 */
struct __attribute__((packed)) service_data_encrypted_state_t {
	uint8_t  id;                  // ID of this stone.
	uint8_t  switchState;
	service_data_state_flags_t flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	int32_t  energyUsed;
	uint16_t partialTimestamp;    // Current timestamp.
	service_data_state_extra_flags_t extraFlags;
	uint8_t  validation;          // Used to check if decryption is successful. Value is always SERVICE_DATA_VALIDATION.
};

/**
 * In case the crownstone has errors, this will be advertised next to the state.
 */
struct __attribute__((packed)) service_data_encrypted_error_t {
	uint8_t  id;                  // ID of this stone.
	uint32_t errors;
	uint32_t timestamp;           // Timestamp of first error.
	service_data_state_flags_t flags;
	int8_t   temperature;
	uint16_t partialTimestamp;    // Current timestamp.
	int16_t  powerUsageReal;
};

/**
 * State of another crownstone.
 */
struct __attribute__((packed)) service_data_encrypted_ext_state_t {
	uint8_t  id;                  // ID of another stone of which this is the state.
	uint8_t  switchState;
	service_data_state_flags_t flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	int32_t  energyUsed;
	uint16_t partialTimestamp;    // Timestamp of when the other stone was in this state.
	int8_t   rssi;                // RSSI between this stone and the other stone.
	uint8_t  validation;          // Used to check if decryption is successful. Value is always SERVICE_DATA_VALIDATION.
};

/**
 * Errors of another crownstone.
 */
struct __attribute__((packed)) service_data_encrypted_ext_error_t {
	uint8_t  id;                  // ID of another stone of which this is the state.
	uint32_t errors;
	uint32_t timestamp;           // Timestamp of first error.
	service_data_state_flags_t flags;
	int8_t   temperature;
	uint16_t partialTimestamp;    // Timestamp of when the other stone was in this state.
	int8_t   rssi;                // RSSI between this stone and the other stone.
	uint8_t  validation;          // Used to check if decryption is successful. Value is always SERVICE_DATA_VALIDATION.
};

/**
 * State of this crownstone.
 *
 * Has some other info than the usual state.
 */
struct __attribute__((packed)) service_data_encrypted_alternative_state_t {
	uint8_t  id;                  // ID of this stone.
	uint8_t  switchState;
	service_data_state_flags_t flags;
	uint16_t behaviourMasterHash;
	uint16_t assetFiltersVersion;
	uint32_t assetFiltersCrc;
	uint16_t partialTimestamp;    // Current timestamp.
	uint8_t  reserved2;
	uint8_t  validation;          // Used to check if decryption is successful. Value is always SERVICE_DATA_VALIDATION.
};


union __attribute__((packed)) service_data_microapp_flags_t {
	struct __attribute__((packed)) {
		bool timeSet : 1; // Whether the time is set.
	} flags;
	uint8_t asInt;
};

/**
 * Microapp data that will be encrypted.
 */
struct __attribute__((packed)) service_data_encrypted_microapp_t {
	service_data_microapp_flags_t flags;
	uint16_t appUuid;             // Identifier picked by the microapp.
	uint8_t data[8];              // Data filled in by the microapp.
	uint8_t id;                   // ID of this stone.
	uint16_t partialTimestamp;    // Required, so that the data keeps changing.
	uint8_t validation;           // Used to check if decryption is successful. Value is always SERVICE_DATA_VALIDATION.
};


enum ServiceDataDataType {
	SERVICE_DATA_DATA_TYPE_STATE = 0,
	SERVICE_DATA_DATA_TYPE_ERROR = 1,
	SERVICE_DATA_DATA_TYPE_EXT_STATE = 2,
	SERVICE_DATA_DATA_TYPE_EXT_ERROR = 3,
	SERVICE_DATA_DATA_TYPE_ALTERNATIVE_STATE = 4,
	SERVICE_DATA_DATA_TYPE_HUB_STATE = 5,
	SERVICE_DATA_DATA_TYPE_MICROAPP = 6,
};

/**
 * This data is encrypted.
 */
struct __attribute__((packed)) service_data_encrypted_t {
	uint8_t type;                 // ServiceDataDataType
	union {
		service_data_encrypted_state_t state;
		service_data_encrypted_error_t error;
		service_data_encrypted_ext_state_t extState;
		service_data_encrypted_ext_error_t extError;
		service_data_encrypted_alternative_state_t altState;
		service_data_hub_state_t hubState;
		service_data_encrypted_microapp_t microapp;
	};
};

/**
 * Function to get the stone ID from the encrypted service data.
 */
constexpr stone_id_t getStoneId(service_data_encrypted_t* data) {
	switch (data->type) {
		case SERVICE_DATA_DATA_TYPE_STATE: return data->state.id;
		case SERVICE_DATA_DATA_TYPE_ERROR: return data->error.id;
		case SERVICE_DATA_DATA_TYPE_EXT_STATE: return data->extState.id;
		case SERVICE_DATA_DATA_TYPE_EXT_ERROR: return data->extError.id;
		case SERVICE_DATA_DATA_TYPE_ALTERNATIVE_STATE: return data->altState.id;
		case SERVICE_DATA_DATA_TYPE_HUB_STATE: return data->hubState.id;
		case SERVICE_DATA_DATA_TYPE_MICROAPP: return data->microapp.id;
	}
	return 0;
}

/**
 * Function to convert from service_data_encrypted_state_t to service_data_encrypted_ext_state_t.
 *
 * Since most fields are at similar places, we don't have to copy much.
 *
 * @param[in,out]  data      Service data that will be converted.
 * @param[in]      rssi      RSSI to the stone, or 0 if unknown / out of reach.
 */
constexpr void convertToExternalState(service_data_encrypted_t* data, int8_t rssi) {
	data->type = SERVICE_DATA_DATA_TYPE_EXT_STATE;
	data->extState.rssi = rssi;
}

/**
 * Function to convert from service_data_encrypted_error_t to service_data_encrypted_ext_error_t.
 *
 * Since most fields are at similar places, we don't have to copy much.
 *
 * @param[in,out]  data      Service data that will be converted.
 * @param[in]      rssi      RSSI to the stone, or 0 if unknown / out of reach.
 */
constexpr void convertToExternalError(service_data_encrypted_t* data, int8_t rssi) {
	data->type = SERVICE_DATA_DATA_TYPE_EXT_ERROR;
	data->extError.rssi = rssi;
}



struct __attribute__((packed)) service_data_setup_state_t {
	uint8_t  switchState;
	service_data_state_flags_t flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	uint32_t errors;
	uint8_t  counter;
	uint8_t  reserved[4];
};

/**
 * Setup data.
 *
 * This is not encrypted.
 */
struct __attribute__((packed)) service_data_setup_t {
	uint8_t type;                 // ServiceDataDataType
	union {
		service_data_setup_state_t state;
		service_data_hub_state_t hubState;
	};
};




// The type of service data: this type is not encrypted.
enum ServiceDataType {
	SERVICE_DATA_TYPE_SETUP = 6,
	SERVICE_DATA_TYPE_ENCRYPTED = 7,
};

/**
 * Service data
 *
 * This is all the data that ends up in the service data field of the advertisement.
 */
union service_data_t {
	struct __attribute__((packed)) {
		uint8_t  type;            // ServiceDataType
		uint8_t  deviceType;      // See cs_DeviceTypes.h
		union {
			service_data_encrypted_t encrypted;
			service_data_setup_t setup;
			uint8_t encryptedArray[sizeof(service_data_encrypted_t)];
		};
	} params;
	uint8_t array[sizeof(params)] = {};
};
