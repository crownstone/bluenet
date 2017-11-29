/**
 * @file
 * Service data.
 *
 * @authors Crownstone Team, Christopher Mason.
 * @copyright Crownstone B.V.
 * @date May 4, 2016
 * @license LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_Settings.h>
#include <storage/cs_State.h>
#include "drivers/cs_Timer.h"
#include "cfg/cs_Config.h"

#include <cstring>

#if BUILD_MESHING == 1
#include <protocol/cs_MeshMessageTypes.h>
#endif

#define SERVICE_DATA_PROTOCOL_VERSION 1

//! bitmask map (if bit is true, then):
//! [0] New Data Available
//! [1] ID shown is from external Crownstone
//! [2] ....
//! [3] ....
//! [4] ....
//! [5] ....
//! [6] ....
//! [7] operation mode setup
enum ServiceBitmask {
	NEW_DATA_AVAILABLE    = 0,
	SHOWING_EXTERNAL_DATA = 1,
	SERVICE_BITMASK_ERROR = 2,
	RESERVED2             = 3,
	RESERVED3             = 4,
	RESERVED4             = 5,
	RESERVED5             = 6,
	SETUP_MODE_ENABLED    = 7
};

//! Service data struct, this data type is what ends up in the advertisement.
union service_data_t {
	struct __attribute__((packed)) {
		uint8_t  protocolVersion;
		uint16_t crownstoneId;
		uint8_t  switchState;
		uint8_t  eventBitmask;
		int8_t   temperature;
		int32_t  powerUsage;
		int32_t  accumulatedEnergy;
		uint8_t  rand;
		uint16_t counter;
	} params;
	uint8_t array[sizeof(params)] = {};
};

class ServiceData : EventListener {

public:
	ServiceData();

	/** Set the power usage field of the service data.
	 *
	 * @param[in] powerUsage      The power usage.
	 */
	void updatePowerUsage(int32_t powerUsage) {
		_serviceData.params.powerUsage = powerUsage;
	}

	/** Set the energy used field of the service data.
	 *
	 * @param[in] energy          The energy used.
	 */
	void updateAccumulatedEnergy(int32_t energy) {
		_serviceData.params.accumulatedEnergy = energy;
	}

	/** Set the ID field of the service data.
	 *
	 * @param[in] crownstoneId    The Crownstone ID.
	 */
	void updateCrownstoneId(uint16_t crownstoneId) {
		_serviceData.params.crownstoneId = crownstoneId;
	}

	/** Set the switch state field of the service data.
	 *
	 * @param[in] switchState     The switch state.
	 */
	void updateSwitchState(uint8_t switchState) {
		_serviceData.params.switchState = switchState;
	}

	/** Set the event bitmask field of the service data.
	 *
	 * @param[in] bitmask         The bitmask.
	 */
	void updateEventBitmask(uint8_t bitmask) {
		_serviceData.params.eventBitmask = bitmask;
	}

	/** Set the temperature field of the service data.
	 *
	 * @param[in] temperature     The temperature.
	 */
	void updateTemperature(int8_t temperature) {
		_serviceData.params.temperature = temperature;
	}

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

	/** Set or unset a bit of the event bitmask.
	 *
	 * @param[in] bit             The bit position.
	 * @param[in] set             True when setting the bit, false when unsetting it.
	 */
	void updateEventBitmask(uint8_t bit, bool set) {
		if (set) {
			_serviceData.params.eventBitmask |= 1 << bit;
		} else {
			_serviceData.params.eventBitmask &= ~(1 << bit);
		}
	}

	/** Get the service data as array.
	 *
	 * @return                    Pointer to the service data array.
	 */
	uint8_t* getArray() {
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED) && _operationMode != OPERATION_MODE_SETUP) {
			return _encryptedArray;
		}
		else {
			return _serviceData.array;
		}

	}

	/** Get the size of the service.
	 *
	 * @return                    Size of the service data.
	 */
	uint16_t getArraySize() {
		return sizeof(service_data_t);
	}

	/** Send the service data over the mesh.
	 *
	 * @param[in] event           True when calling this function because the state changed significantly.
	 */
	void sendMeshState(bool event);

private:
	//! Timer used to periodically update the advertisement.
	app_timer_t    _updateTimerData;
	app_timer_id_t _updateTimerId;

	//! Store the service data
	service_data_t _serviceData;

	//! Used to store the service data of an external Crownstone.
	service_data_t _serviceDataExt;

	//! Used to store the encrypted service data.
	union {
		struct __attribute__((packed)) {
			uint8_t  protocolVersion;
			uint8_t  payload[16];
		} _encryptedParams;
		uint8_t _encryptedArray[sizeof(service_data_t)] = {};
	};

	//! Store the operation mode.
	uint8_t _operationMode;
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
	bool getExternalAdvertisement(uint16_t ownId, service_data_t& serviceData);

	/** Called when there are events to handle.
	 *
	 * @param[in] evt             Event type, see cs_EventTypes.h.
	 * @param[in] p_data          Pointer to the data.
	 * @param[in] length          Length of the data.
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

#if BUILD_MESHING == 1
	//! Timer used to periodically send the state to the mesh.
	app_timer_t    _meshStateTimerData;
	app_timer_id_t _meshStateTimerId;

	struct __attribute__((packed)) advertised_ids_t {
		uint8_t size;
		int8_t head; // Index of last crownstone ID that was advertised
		id_type_t list[MESH_STATE_HANDLE_COUNT * MAX_STATE_ITEMS];
	};

	struct __attribute__((packed)) last_seen_id_t {
		uint16_t id;
		uint32_t timestamp;
		uint16_t hash;
		uint8_t  timedout; // 0 when not timed out, 1 when timed out.
		                   // We need this, or a clock overflow could mark a state as not timed out again.
	};

	//! List of external crownstone IDs that have been advertised, used to determine which external crownstone to advertise.
	advertised_ids_t _advertisedIds;

	//! List of external crownstone IDs with timestamp when they were last seen, and a hash of its data.
	last_seen_id_t _lastSeenIds[MESH_STATE_HANDLE_COUNT][LAST_SEEN_COUNT_PER_STATE_CHAN];

	/** Number of times we should still only advertise states triggered by events.
	 *  This is set to some number when a 'state triggered by event' was received, and then counts down.
	 */
	uint8_t _numAdvertiseChangedStates;

	/** Process a received mesh state message.
	 *
	 * Checks whether the last state was triggered by event.
	 *
	 * @param[in] ownId           Id of this Crownstone.
	 * @param[in] stateMsg        Pointer to the mesh state message.
	 * @param[in] stateChan       Which state channel the message was received.
	 */
	void onMeshStateMsg(id_type_t ownId, state_message_t* stateMsg, uint16_t stateChan);

	/** Process a mesh state item, keeps up the last seen table.
	 *
	 * When the id of this state is already in the table: update the timestamp, if the hash is different.
	 * Otherwise overwrite a spot that has id 0, is timed out, or has the oldest timestamp (in that order).
	 *
	 * @param[in] ownId           Id of this Crownstone.
	 * @param[in] stateMsg        Pointer to the mesh state message.
	 * @param[in] stateChan       Which state channel the message was received.
	 */
	void onMeshStateSeen(id_type_t ownId, state_item_t* stateItem, uint16_t stateChan);

	/** Check if the state of given id is considered timed out.
	 *
	 * The state of a Crownstone is considered not timed out when:
	 * a) It's in the table with a recent timestamp, or b) when all entries in the table are not timed out.
	 *
	 * @param[in] id              Id to check.
	 * @param[in] stateChan       On which state channel given id is found.
	 * @param[in] currentTime     Current time (RTC ticks).
	 * @return                    True when state is not considered timed out.
	 */
	bool isMeshStateNotTimedOut(id_type_t id, uint16_t stateChan, uint32_t currentTime);

	/** Choose an external id to advertise the state of.
	 *
	 * @param[in] ownId           Id of this Crownstone.
	 * @param[in] stateMsgs       All the current state messages in the mesh.
	 * @param[in] hasStateMsg     Whether or not the state messages are valid.
	 * @param[in] eventOnly       True when only allowed to choose states which were triggered by event.
	 * @return                    The chosen id.
	 */
	id_type_t chooseExternalId(id_type_t ownId, state_message_t stateMsgs[], bool hasStateMsg[], bool eventOnly);

	/* Static function for the timeout */
	static void meshStateTick(ServiceData *ptr) {
		ptr->sendMeshState(false);
	}
#endif

};

