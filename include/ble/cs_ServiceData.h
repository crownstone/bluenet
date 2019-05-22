/**
 * @file
 * Service data.
 *
 * @authors Crownstone Team, Christopher Mason.
 * @copyright Crownstone B.V.
 * @date May 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include "drivers/cs_Timer.h"
#include "cfg/cs_Config.h"

#include <cstring>

#define SERVICE_DATA_VALIDATION 0xFA

enum ServiceDataFlagBits {
	SERVICE_DATA_FLAGS_DIMMING_AVAILABLE   = 0,
	SERVICE_DATA_FLAGS_MARKED_DIMMABLE     = 1,
	SERVICE_DATA_FLAGS_ERROR               = 2,
	SERVICE_DATA_FLAGS_SWITCH_LOCKED       = 3,
	SERVICE_DATA_FLAGS_TIME_SET            = 4,
	SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED = 5,
	SERVICE_DATA_FLAGS_RESERVED6           = 6,
	SERVICE_DATA_FLAGS_RESERVED7           = 7
};

enum ServiceDataUnencryptedType {
	SERVICE_DATA_TYPE_ENCRYPTED = 5,
	SERVICE_DATA_TYPE_SETUP = 6,
};

enum ServiceDataEncryptedType {
	SERVICE_DATA_TYPE_STATE = 0,
	SERVICE_DATA_TYPE_ERROR = 1,
	SERVICE_DATA_TYPE_EXT_STATE = 2,
	SERVICE_DATA_TYPE_EXT_ERROR = 3,
};

struct __attribute__((packed)) service_data_encrypted_state_t {
	uint8_t  id;
	uint8_t  switchState;
	uint8_t  flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	int32_t  energyUsed;
	uint16_t partialTimestamp;
	uint8_t  reserved;
	uint8_t  validation;
};

struct __attribute__((packed)) service_data_encrypted_error_t {
	uint8_t  id;
	uint32_t errors;
	uint32_t timestamp;
	uint8_t  flags;
	int8_t   temperature;
	uint16_t partialTimestamp;
	int16_t  powerUsageReal;
};

struct __attribute__((packed)) service_data_encrypted_ext_state_t {
	uint8_t  id;
	uint8_t  switchState;
	uint8_t  flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	int32_t  energyUsed;
	uint16_t partialTimestamp;
//	uint8_t  reserved[2];
//	uint16_t validation;
	int8_t   rssi;
	uint8_t  validation;
};

struct __attribute__((packed)) service_data_encrypted_ext_error_t {
	uint8_t  id;
	uint32_t errors;
	uint32_t timestamp;
	uint8_t  flags;
	int8_t   temperature;
	uint16_t partialTimestamp;
//	uint8_t  reserved[2];
//	uint16_t validation;
	int8_t   rssi;
	uint8_t  validation;
};

struct __attribute__((packed)) service_data_encrypted_t {
	uint8_t type;
	union {
		service_data_encrypted_state_t state;
		service_data_encrypted_error_t error;
		service_data_encrypted_ext_state_t extState;
		service_data_encrypted_ext_error_t extError;
	};
};



struct __attribute__((packed)) service_data_setup_state_t {
	uint8_t  switchState;
	uint8_t  flags;
	int8_t   temperature;
	int8_t   powerFactor;
	int16_t  powerUsageReal;
	uint32_t errors;
	uint8_t  counter;
	uint8_t  reserved[4];
};

struct __attribute__((packed)) service_data_setup_t {
	uint8_t type;
	union {
		service_data_setup_state_t state;
	};
};

//! Service data struct, this data type is what ends up in the advertisement.
union service_data_t {
	struct __attribute__((packed)) {
		uint8_t  protocolVersion;
		uint8_t  deviceType;
		union {
			service_data_encrypted_t encrypted;
			service_data_setup_t setup;
			uint8_t encryptedArray[sizeof(service_data_encrypted_t)];
		};
	} params;
	uint8_t array[sizeof(params)] = {};
};

class ServiceData : EventListener {

public:
	ServiceData();

	/** Init the service data, make sure you set important fields first.
	 */
	void init();

	/** Set the device type field of the service data.
	 *
	 * @param[in] deviceType      The device type, see cs_DeviceTypes.h
	 */
	void setDeviceType(uint8_t deviceType);

	/** Set the power usage field of the service data.
	 *
	 * @param[in] powerUsage      The power usage in milliWatt.
	 */
	void updatePowerUsage(int32_t powerUsage);

	/** Set the energy used field of the service data.
	 *
	 * @param[in] energy          The energy used in units of 64 Joule.
	 */
	void updateAccumulatedEnergy(int32_t energy);

	/** Set the ID field of the service data.
	 *
	 * @param[in] crownstoneId    The Crownstone ID.
	 */
	void updateCrownstoneId(stone_id_t crownstoneId);

	/** Set the switch state field of the service data.
	 *
	 * @param[in] switchState     The switch state.
	 */
	void updateSwitchState(uint8_t switchState);

	/** Set the event bitmask field of the service data.
	 *
	 * @param[in] bitmask         The bitmask.
	 */
	void updateFlagsBitmask(uint8_t bitmask);

	/** Set or unset a bit of the event bitmask.
	 *
	 * @param[in] bit             The bit position.
	 * @param[in] set             True when setting the bit, false when unsetting it.
	 */
	void updateFlagsBitmask(uint8_t bit, bool set);

	/** Set the temperature field of the service data.
	 *
	 * @param[in] temperature     The temperature.
	 */
	void updateTemperature(int8_t temperature);

	/** Set the cached state errors
	 *
	 * @param[in] type            Type of error
	 * @param[in] set             Whether to set the error.
	 */
	void updateStateErrors(CS_TYPE type, bool set);

	/** Update the advertisement.
	 *
	 * Update the advertisement with the latest data, and encrypt the data.
	 * When meshing is enabled, the data of external Crownstones can be put in the advertisement instead.
	 * Sends out event EVT_ADVERTISEMENT_UPDATED.
	 *
	 * @param[in] initial         Set initial to true when this is just the initial data
	 *                            when there's no need to send out the event.
	 */
	void updateAdvertisement(bool initial);

	/** Get the service data as array.
	 *
	 * @return                    Pointer to the service data array.
	 */
	uint8_t* getArray();

	/** Get the size of the service.
	 *
	 * @return                    Size of the service data.
	 */
	uint16_t getArraySize();

private:
	//! Timer used to periodically update the advertisement.
	app_timer_t    _updateTimerData;
	app_timer_id_t _updateTimerId;

	//! Stores the last (current) advertised service data
	service_data_t _serviceData;

	//! Store own ID
	stone_id_t _crownstoneId; // TODO: use State for this?

	//! Store switch state
	uint8_t _switchState; // TODO: use State for this?

	//! Store flags
	uint8_t _flags; // TODO: use State for this?

	//! Store the temperature
	int8_t  _temperature; // TODO: use State for this?

	//! Store the power factor
	int8_t  _powerFactor; // TODO: use State for this?

	//! Store the power usage in mW
	int32_t _powerUsageReal; // TODO: use State for this?

	//! Store the energy used, in units of 64 J
	int32_t _energyUsed; // TODO: use State for this?

	//! Store timestamp of first error
	uint32_t _firstErrorTimestamp; // TODO: use State for this?

	uint32_t _sendStateCountdown = MESH_SEND_STATE_INTERVAL_MS / TICK_INTERVAL_MS;

//	//! Store the error state, so that they don't have to be retrieved every time.
//	state_errors_t _stateErrors;

	//! Store the operation mode.
	OperationMode _operationMode;

	//! Store whether a device is connected or not.
	bool _connected;

	//! Counter that keeps up the number of times that the advertisement has been updated.
	uint32_t _updateCount;

	/* Static function for the timeout */
	static void staticTimeout(ServiceData *ptr) {
		ptr->updateAdvertisement(false);
	}

	/** Decide which external Crownstone's state to advertise.
	 *
	 * @param[in] ownId           Id of this Crownstone.
	 * @param[out] serviceData    Service data to fill with the state of the selected Crownstone.
	 * @return                    True when Crownstone was selected and service data has been filled.
	 */
	bool getExternalAdvertisement(stone_id_t ownId, service_data_t& serviceData);

	/** Called when there are events to handle.
	 *
	 * @param[in] evt             Event type, see cs_EventTypes.h.
	 * @param[in] p_data          Pointer to the data.
	 * @param[in] length          Length of the data.
	 */
	void handleEvent(event_t & event);



	/** Compress power usage, according to service data protocol v3.
	 *
	 * @param[in] powerUsageMW    Power usage in milliWatt
	 * @return                    Compressed representation of the power usage.
	 */
	int16_t compressPowerUsageMilliWatt(int32_t powerUsageMW);

	/** Decompress power usage, according to service data protocol v3.
	 *
	 * @param[in] powerUsageMW    Compressed representation of the power usage.
	 * @return                    Power usage in milliWatt.
	 */
	int32_t decompressPowerUsage(int16_t compressedPowerUsage);

	/** Convert energy used from service data protocol v3 to service data protocol v1.
	 *
	 * @param[in] energyUsed      The accumulated energy in units of 64J.
	 * @return                    The accumulated energy in units of Wh.
	 */
	int32_t convertEnergyV3ToV1(int32_t energyUsed);

	/** Convert energy used from service data protocol v1 to service data protocol v3.
	 *
	 * @param[in] energyUsed      The accumulated energy in units of Wh.
	 * @return                    The accumulated energy in units of 64J.
	 */
	int32_t convertEnergyV1ToV3(int32_t energyUsed);

	/** Convert complete energy usage value to a partial energy usage value, according to service data protocol v3.
	 *
	 * @param[in] energyUsed      The accumulated energy in units of 64J.
	 * @return                    The least significant part of the energy usage.
	 */
	uint16_t energyToPartialEnergy(int32_t energyUsed);

	/** Convert partial energy usage value to a complete energy usage value, according to service data protocol v3.
	 *
	 * @param[in] partialEnergy   The least significant part of the energy usage.
	 * @return                    Energy usage.
	 */
	int32_t partialEnergyToEnergy (uint16_t partialEnergy);

	/** Convert timestamp to a partial timestamp, according to service data protocol v3.
	 *
	 * @param[in] timestamp       The timestamp.
	 * @return                    The least significant part of the timestamp.
	 */
	uint16_t timestampToPartialTimestamp(uint32_t timestamp);


	/** When a timestamp is available, return partial timestamp, else returns a counter.
	 *
	 * @param[in] timestamp       The timestamp.
	 * @param[in] counter         The counter.
	 *
	 * @return                    Counter, or the least significant part of the timestamp.
	 */
	uint16_t getPartialTimestampOrCounter(uint32_t timestamp, uint32_t counter);

	/** Send the state over the mesh.
	 */
	void sendMeshState();
};

