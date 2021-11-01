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

#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventDispatcher.h>
#include <events/cs_EventListener.h>
#include <processing/cs_ExternalStates.h>
#include <storage/cs_State.h>

class ServiceData : EventListener {

public:
	ServiceData();

	/**
	 * Init the service data, make sure you set important fields first.
	 *
	 * @param[in] deviceType      The device type, see cs_DeviceTypes.h
	 */
	void init(uint8_t deviceType);

	/**
	 * Set the device type field of the service data.
	 *
	 * @param[in] deviceType      The device type, see cs_DeviceTypes.h
	 */
	void setDeviceType(uint8_t deviceType);

	/**
	 * Set the power usage field of the service data.
	 *
	 * @param[in] powerUsage      The power usage in milliWatt.
	 */
	void updatePowerUsage(int32_t powerUsage);

	/**
	 * Set the energy used field of the service data.
	 *
	 * @param[in] energy          The energy used in units of 64 Joule.
	 */
	void updateAccumulatedEnergy(int32_t energy);

	/**
	 * Set the ID field of the service data.
	 *
	 * @param[in] crownstoneId    The Crownstone ID.
	 */
	void updateCrownstoneId(stone_id_t crownstoneId);

	/**
	 * Set the switch state field of the service data.
	 *
	 * @param[in] switchState     The switch state.
	 */
	void updateSwitchState(uint8_t switchState);

	/**
	 * Set the temperature field of the service data.
	 *
	 * @param[in] temperature     The temperature.
	 */
	void updateTemperature(int8_t temperature);

	/**
	 * Update the service data.
	 *
	 * Updates some states.
	 * Selects a type of data, and puts this in the service data.
	 * Writes the service data to UART.
	 * Sends out event EVT_ADVERTISEMENT_UPDATED.
	 *
	 * @param[in] initial         Set initial to true when this is just the initial data
	 *                            when there's no need to send out the event.
	 */
	void updateServiceData(bool initial);

	/**
	 * Get the service data as array.
	 */
	uint8_t* getArray();

	/**
	 * Get the size of the service data.
	 */
	uint16_t getArraySize();

private:
	//! Timer used to periodically update the advertisement.
	app_timer_t    _updateTimerData;
	app_timer_id_t _updateTimerId = NULL;

	/**
	 * Stores the last (current) advertised service data.
	 *
	 * This data will be copied by the advertiser.
	 */
	service_data_t _serviceData;

	//! Cache own ID
	stone_id_t _crownstoneId = 0;

	//! Cache switch state
	uint8_t _switchState = 0;

	//! Cache flags
	service_data_state_flags_t _flags;

	//! Cache extra flags
	service_data_state_extra_flags_t _extraFlags;

	//! Cache the temperature
	int8_t  _temperature = 0;

	//! Cache the power factor
	int8_t  _powerFactor = 0;

	//! Cache the power usage in mW
	int32_t _powerUsageReal = 0;

	//! Cache the energy used, in units of 64 J
	int32_t _energyUsed = 0;

	//! Cache the state errors.
	TYPIFY(STATE_ERRORS) _stateErrors;

	//! Cache timestamp of first error
	uint32_t _firstErrorTimestamp = 0;

	uint32_t _sendStateCountdown = MESH_SEND_STATE_INTERVAL_MS / TICK_INTERVAL_MS;

	//! Cache the operation mode.
	OperationMode _operationMode = OperationMode::OPERATION_MODE_UNINITIALIZED;

	//! Counter that keeps up the number of times that the advertisement has been updated.
	uint32_t _updateCount = 0;

	ExternalStates _externalStates;

	/**
	 * Whether the microapp wants to advertise service data.
	 */
	bool _microappServiceDataSet = false;

	/**
	 * Microapp data to be advertised in crownstone service data.
	 */
	service_data_encrypted_microapp_t _microappServiceData;

	/* Static function for the timeout */
	static void staticTimeout(ServiceData *ptr) {
		ptr->updateServiceData(false);
	}

	/**
	 * Selects a type of data and puts this in the service data.
	 *
	 * @return     True when service data has to be encrypted.
	 */
	bool fillServiceData(uint32_t timestamp);

	/**
	 * Encrypt the service data.
	 */
	void encryptServiceData();

	/**
	 * Put the state of this Crownstone in setup mode in the service data.
	 */
	void fillWithSetupState(uint32_t timestamp);

	/**
	 * Put the state of this Crownstone in the service data.
	 */
	void fillWithState(uint32_t timestamp);

	/**
	 * Put the error state of this Crownstone in the service data.
	 */
	void fillWithError(uint32_t timestamp);

	/**
	 * Put the state or error state of another Crownstone in the service data.
	 *
	 * @return     True when Crownstone service data has been filled.
	 */
	bool fillWithExternalState();

	/**
	 * Put the alternative state of this Crownstone in the service data.
	 */
	void fillWithAlternativeState(uint32_t timestamp);

	/**
	 * Put the hub state in the service data.
	 */
	void fillWithHubState(uint32_t timestamp);

	/**
	 * Put microapp data in the service data.
	 *
	 * @return     True when Crownstone service data has been filled.
	 */
	bool fillWithMicroapp(uint32_t timestamp);


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

	/**
	 * Get a part of the behaviour hash.
	 */
	uint16_t getPartialBehaviourHash(uint32_t behaviourHash);

	/** Send the state over the mesh.
	 *
	 * @param[in] event           True when sending state because of an event.
	 */
	void sendMeshState(bool event);
};

