/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include "ble/cs_ServiceData.h"

#include "processing/cs_EncryptionHandler.h"
#include "protocol/cs_StateTypes.h"
#include "protocol/cs_ConfigTypes.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_RNG.h"
#include "storage/cs_State.h"
#include "util/cs_Utils.h"
#include "drivers/cs_RTC.h"
#include "protocol/cs_UartProtocol.h"

#if BUILD_MESHING == 1
#include "mesh/cs_MeshControl.h"
#include "protocol/mesh/cs_MeshMessageState.h"
//#include "mesh/cs_Mesh.h"
#endif

#define ADVERTISE_EXTERNAL_DATA
//#define PRINT_DEBUG_EXTERNAL_DATA
//#define PRINT_VERBOSE_EXTERNAL_DATA

ServiceData::ServiceData() :
	_updateTimerId(NULL)
	,_crownstoneId(0)
	,_switchState(0)
	,_flags(0)
	,_temperature(0)
	,_powerFactor(0)
	,_powerUsageReal(0)
	,_energyUsed(0)
	,_firstErrorTimestamp(0)
	,_connected(false)
	,_updateCount(0)
#if BUILD_MESHING == 1
	,_meshSendCount(0)
	,_meshLastSentTimestamp(0)
	,_meshNextEventType(0)
#endif
{
	// Initialize the service data
	memset(_serviceData.array, 0, sizeof(_serviceData.array));
};

void ServiceData::init() {
	// we want to update the advertisement packet on a fixed interval.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	// get the operation mode from state
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

	EventDispatcher::getInstance().addListener(this);

	// start the update timer
	Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);

#if BUILD_MESHING == 1
	_meshStateTimerData = { {0} };
	_meshStateTimerId = &_meshStateTimerData;
	Timer::getInstance().createSingleShot(_meshStateTimerId, (app_timer_timeout_handler_t)ServiceData::meshStateTick);

	// Init _lastSeenIds
	memset(&(_lastSeenIds[0][0]), 0, MESH_STATE_HANDLE_COUNT * LAST_SEEN_COUNT_PER_STATE_CHAN * sizeof(last_seen_id_t));

	// Only send the state over the mesh in normal mode
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED) && _operationMode == OPERATION_MODE_NORMAL) {
		// Start the mesh state timer with a small random delay
		// Make sure delay is not 0, as that's an invalid delay.

//		// Delay is 1-59926 ms
//		uint8_t rand8;
//		RNG::fillBuffer(&rand8, 8);
//		uint32_t randMs = rand8*235 + 1;

		// Delay is 1-65537 ms
		uint16_t rand16;
		RNG::fillBuffer((uint8_t*)&rand16, 2);
		uint32_t randMs = rand16 + 1;

		Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(randMs), this);
	}
#endif

#if BUILD_MESHING == 1
	//! Init advertisedIds
	_advertisedIds.size = 0;
	_advertisedIds.head = 0;
	_numAdvertiseChangedStates = 0;
#endif

#if BUILD_MESHING == 1
	LOGd("Servicedata size: %u", sizeof(service_data_t));
	LOGd("State item size: %u", sizeof(state_item_t));
	LOGd("State item list size: %u", MAX_STATE_ITEMS);
#endif

	// Init flags
	updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, Settings::getInstance().isSet(CONFIG_PWM_ALLOWED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED, Settings::getInstance().isSet(CONFIG_SWITCHCRAFT_ENABLED));

	// set the initial advertisement.
	updateAdvertisement(true);
}

void ServiceData::setDeviceType(uint8_t deviceType) {
//	_deviceType = deviceType;
	_serviceData.params.deviceType = deviceType;
}

void ServiceData::updatePowerUsage(int32_t powerUsage) {
	_powerUsageReal = powerUsage;
	_powerFactor = 127;
}

void ServiceData::updateAccumulatedEnergy(int32_t energy) {
	_energyUsed = energy;
}

void ServiceData::updateCrownstoneId(uint8_t crownstoneId) {
	_crownstoneId = crownstoneId;
}

void ServiceData::updateSwitchState(uint8_t switchState) {
	_switchState = switchState;
}

void ServiceData::updateFlagsBitmask(uint8_t bitmask) {
	_flags = bitmask;
}

void ServiceData::updateFlagsBitmask(uint8_t bit, bool set) {

	if (set) {
//		_flags |= 1 << bit;
		BLEutil::setBit(_flags, bit);
	}
	else {
//		_flags &= ~(1 << bit);
		BLEutil::clearBit(_flags, bit);
	}
}

void ServiceData::updateTemperature(int8_t temperature) {
	_temperature = temperature;
}

uint8_t* ServiceData::getArray() {
	return _serviceData.array;
}

uint16_t ServiceData::getArraySize() {
	return sizeof(service_data_t);
}


void ServiceData::updateAdvertisement(bool initial) {
	Timer::getInstance().stop(_updateTimerId);

	// if we are connected, we do not need to update the packet.
	if (_connected == false) {
		_updateCount++;

		bool serviceDataSet = false;

		state_errors_t stateErrors;
		State::getInstance().get(STATE_ERRORS, stateErrors.asInt);

		uint32_t timestamp;
		State::getInstance().get(STATE_TIME, timestamp);

//		// Update flag
//		updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, Settings::getInstance().isSet(CONFIG_PWM_ALLOWED));
//		updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED));

		// Set error timestamp
		if (stateErrors.asInt == 0) {
			_firstErrorTimestamp = 0;
		}
		else if (_firstErrorTimestamp != 0) {
			_firstErrorTimestamp = timestamp;
		}

		if (_operationMode == OPERATION_MODE_SETUP) {
			// In setup mode, only advertise this state.
			_serviceData.params.protocolVersion = SERVICE_DATA_TYPE_SETUP;
			_serviceData.params.setup.type = 0;
			_serviceData.params.setup.state.switchState = _switchState;
			_serviceData.params.setup.state.flags = _flags;
			_serviceData.params.setup.state.temperature = _temperature;
			_serviceData.params.setup.state.powerFactor = _powerFactor;
			_serviceData.params.setup.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
			_serviceData.params.setup.state.errors = stateErrors.asInt;
			_serviceData.params.setup.state.counter = _updateCount;
			memset(_serviceData.params.setup.state.reserved, 0, sizeof(_serviceData.params.setup.state.reserved));
			serviceDataSet = true;
		}

		// Every 2 updates, we advertise the errors (if any), or the state of another crownstone.
		if (!serviceDataSet && _updateCount % 2 == 0) {
			if (stateErrors.asInt != 0) {
				_serviceData.params.protocolVersion = SERVICE_DATA_TYPE_ENCRYPTED;
				_serviceData.params.encrypted.type = SERVICE_DATA_TYPE_ERROR;
				_serviceData.params.encrypted.error.id = _crownstoneId;
				_serviceData.params.encrypted.error.errors = stateErrors.asInt;
				_serviceData.params.encrypted.error.timestamp = _firstErrorTimestamp;
				_serviceData.params.encrypted.error.flags = _flags;
				_serviceData.params.encrypted.error.temperature = _temperature;
				_serviceData.params.encrypted.error.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
				_serviceData.params.encrypted.error.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
				serviceDataSet = true;
			}
			else if (getExternalAdvertisement(_crownstoneId, _serviceData)) {
				serviceDataSet = true;
			}
		}

		if (!serviceDataSet) {
			_serviceData.params.protocolVersion = SERVICE_DATA_TYPE_ENCRYPTED;
			_serviceData.params.encrypted.type = SERVICE_DATA_TYPE_STATE;
			_serviceData.params.encrypted.state.id = _crownstoneId;
			_serviceData.params.encrypted.state.switchState = _switchState;
			_serviceData.params.encrypted.state.flags = _flags;
			_serviceData.params.encrypted.state.temperature = _temperature;
			_serviceData.params.encrypted.state.powerFactor = _powerFactor;
			_serviceData.params.encrypted.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
			_serviceData.params.encrypted.state.energyUsed = _energyUsed;
			_serviceData.params.encrypted.state.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
			_serviceData.params.encrypted.state.reserved = 0;
			_serviceData.params.encrypted.state.validation = SERVICE_DATA_VALIDATION;
		}

#ifdef PRINT_DEBUG_EXTERNAL_DATA
		LOGd("servideData:");
		BLEutil::printArray(_serviceData.array, sizeof(service_data_t));
//		LOGd("serviceData: type=%u id=%u switch=%u bitmask=%u temp=%i P=%i E=%i time=%u", serviceData->params.type, serviceData->params.crownstoneId, serviceData->params.switchState, serviceData->params.flagBitmask, serviceData->params.temperature, serviceData->params.powerUsageReal, serviceData->params.accumulatedEnergy, serviceData->params.partialTimestamp);
#endif
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_SERVICE_DATA, _serviceData.array, sizeof(service_data_t));

//		Mesh::getInstance().printRssiList();

		// encrypt the array using the guest key ECB if encryption is enabled.
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED) && _operationMode != OPERATION_MODE_SETUP) {
			EncryptionHandler::getInstance().encrypt(
					_serviceData.params.encryptedArray, sizeof(_serviceData.params.encryptedArray),
					_serviceData.params.encryptedArray, sizeof(_serviceData.params.encryptedArray),
					GUEST, ECB_GUEST);
//			EncryptionHandler::getInstance().encrypt((_serviceData.array) + 1, sizeof(service_data_t) - 1, _encryptedParams.payload,
//			                                         sizeof(_encryptedParams.payload), GUEST, ECB_GUEST);
		}

		if (!initial) {
			EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_UPDATED);
		}
	}

	// start the timer again.
	if (_operationMode == OPERATION_MODE_SETUP) {
		Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD_SETUP), this);
	}
	else {
		Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);
	}
}

bool ServiceData::getExternalAdvertisement(stone_id_t ownId, service_data_t& serviceData) {
#if BUILD_MESHING == 1 && defined(ADVERTISE_EXTERNAL_DATA)

	if (_operationMode != OPERATION_MODE_NORMAL || Settings::getInstance().isSet(CONFIG_MESH_ENABLED) == false) {
		return false;
	}

	uint32_t currentTime = RTC::getCount();

	// TODO: don't put multiple messages on the stack. But getLastStateDataMessage() is also expensive..
	state_message_t messages[MESH_STATE_HANDLE_COUNT];
	bool hasStateMsg[MESH_STATE_HANDLE_COUNT];
	uint16_t messageSize = sizeof(state_message_t);

	// First check if there's any valid state message, and keep up which channels have a valid message
	bool anyStateMsg = false;
	for (int chan=0; chan<MESH_STATE_HANDLE_COUNT; ++chan) {
		hasStateMsg[chan] = MeshControl::getInstance().getLastStateDataMessage(messages[chan], messageSize, chan);
		if (hasStateMsg[chan] && is_valid_state_msg(&(messages[chan])) && messages[chan].size) {
			anyStateMsg = true;
		}
		else {
			hasStateMsg[chan] = false;
		}
	}
	if (!anyStateMsg) {
		return false;
	}

	stone_id_t advertiseId = 0;

#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("getExternalAdvertisement changed=%u", _numAdvertiseChangedStates);
#endif
	if (_numAdvertiseChangedStates > 0) {
		// Select an id which has a state triggered by an event.
		advertiseId = chooseExternalId(ownId, messages, hasStateMsg, true);
		_numAdvertiseChangedStates--;
	}
	if (advertiseId == 0) {
		// Didn't select an id which has a state triggered by an event, so select a regular one.
		_numAdvertiseChangedStates = 0;
		advertiseId = chooseExternalId(ownId, messages, hasStateMsg, false);
	}

	if (advertiseId == 0) {
		return false;
	}

	// Fill the service data with the data of the selected id
	bool advertise = true;
	bool found = false;
	for (uint8_t chan=0; chan<MESH_STATE_HANDLE_COUNT; ++chan) {
		if (!hasStateMsg[chan]) {
			continue;
		}

		int16_t idx = -1;
		state_item_t* stateItem;
		while (peek_prev_state_item(&(messages[chan]), &stateItem, idx)) {
			stone_id_t itemId = meshStateItemGetId(stateItem);
			if (itemId == advertiseId) {
				if (!isMeshStateNotTimedOut(advertiseId, chan, currentTime)) {
					advertise = false;
				}
				switch (stateItem->type) {
				case MESH_STATE_ITEM_TYPE_STATE:
				case MESH_STATE_ITEM_TYPE_EVENT_STATE: {
					serviceData.params.encrypted.type = SERVICE_DATA_TYPE_EXT_STATE;
					serviceData.params.encrypted.extState.id = stateItem->state.id;
					serviceData.params.encrypted.extState.switchState = stateItem->state.switchState;
					serviceData.params.encrypted.extState.flags = stateItem->state.flags;
					serviceData.params.encrypted.extState.temperature = stateItem->state.temperature;
					serviceData.params.encrypted.extState.powerFactor = stateItem->state.powerFactor;
					serviceData.params.encrypted.extState.powerUsageReal = stateItem->state.powerUsageReal;
					serviceData.params.encrypted.extState.energyUsed = stateItem->state.energyUsed;
					serviceData.params.encrypted.extState.partialTimestamp = stateItem->state.partialTimestamp;
//					memset(serviceData.params.encrypted.extState.reserved, 0, sizeof(serviceData.params.encrypted.extState.reserved));
//					serviceData.params.encrypted.extState.validation = 0xFACE;
					serviceData.params.encrypted.extState.rssi = MeshControl::getInstance().getRssi(stateItem->state.id);
					serviceData.params.encrypted.extState.validation = SERVICE_DATA_VALIDATION;
					break;
				}
				case MESH_STATE_ITEM_TYPE_ERROR: {
					serviceData.params.encrypted.type = SERVICE_DATA_TYPE_EXT_ERROR;
					serviceData.params.encrypted.extError.id = stateItem->error.id;
					serviceData.params.encrypted.extError.errors = stateItem->error.errors;
					serviceData.params.encrypted.extError.timestamp = stateItem->error.timestamp;
					serviceData.params.encrypted.extError.flags = stateItem->error.flags;
					serviceData.params.encrypted.extError.temperature = stateItem->error.temperature;
					serviceData.params.encrypted.extError.partialTimestamp = stateItem->error.partialTimestamp;
//					memset(serviceData.params.encrypted.extError.reserved, 0, sizeof(serviceData.params.encrypted.extError.reserved));
//					serviceData.params.encrypted.extError.validation = 0xFACE;
					serviceData.params.encrypted.extState.rssi = MeshControl::getInstance().getRssi(stateItem->state.id);
					serviceData.params.encrypted.extState.validation = SERVICE_DATA_VALIDATION;
					break;
				}
				default:
					advertise = false;
				}

				found = true;
				break;
			}
		}
		if (found) {
			break;
		}
	}

	if (!found || !advertise) {
		LOGw("Not advertising %u", advertiseId);
	}
#ifdef PRINT_DEBUG_EXTERNAL_DATA
	else {
//		LOGd("serviceData: type=%u id=%u switch=%u bitmask=%u temp=%i P=%i E=%i time=%u adv=%u", serviceData.params.type, serviceData.params.crownstoneId, serviceData.params.switchState, serviceData.params.flagBitmask, serviceData.params.temperature, serviceData.params.powerUsageReal, serviceData.params.accumulatedEnergy, serviceData.params.partialTimestamp, advertise);
		LOGd("ext serviceData:");
		BLEutil::printArray(serviceData.array, sizeof(serviceData));
	}
#endif

	// If we advertised the id at head, then increase the head
	if (advertiseId == _advertisedIds.list[_advertisedIds.head]) {
		_advertisedIds.head = (_advertisedIds.head + 1) % _advertisedIds.size;
	}

	return advertise && found;
#else
	return false;
#endif
}

#if BUILD_MESHING == 1
stone_id_t ServiceData::chooseExternalId(stone_id_t ownId, state_message_t stateMsgs[], bool hasStateMsg[], bool eventOnly) {

#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("chooseExternalId");
	LOGd("head=%u list:", _advertisedIds.head);
	for (uint8_t i=0; i<_advertisedIds.size; ++i) {
		_logSerial(SERIAL_DEBUG, " %u", _advertisedIds.list[i]);
	}
	_logSerial(SERIAL_DEBUG, SERIAL_CRLF);

	LOGd("msgs:");
	for (uint8_t chan=0; chan<MESH_STATE_HANDLE_COUNT; ++chan) {
		if (!hasStateMsg[chan]) {
			continue;
		}
		int16_t idx = -1;
		state_item_t* stateItem;
		while (peek_next_state_item(&(stateMsgs[chan]), &stateItem, idx)) {
			stone_id_t __attribute__((unused)) itemId = meshStateItemGetId(stateItem);
			_logSerial(SERIAL_DEBUG, "  id=%u type=%u", itemId, stateItem->type);
		}
	}
	_logSerial(SERIAL_DEBUG, SERIAL_CRLF);
#endif

	// Remove ids from the recent list that are not in the messages
	advertised_ids_t tempAdvertisedIds;
	tempAdvertisedIds.size = 0;

	// First copy the ids that are in both the recent list and the message, to a temp list.
	// Make the recent list the outer loop, so that we don't get any doubles in the temp list.
	uint8_t i;
	for (i=0; i<_advertisedIds.size; ++i) {
		bool found = false;

		for (uint8_t chan=0; chan<MESH_STATE_HANDLE_COUNT; ++chan) {
			if (!hasStateMsg[chan]) {
				continue;
			}

//			state_message stateMsgWrapper(&message);
//			for (state_message::iterator iter = stateMsgWrapper.begin(); iter != stateMsgWrapper.end(); ++iter) {
//				state_item_t stateItem = *iter;
			int16_t idx = -1;
			state_item_t* stateItem;
			while (peek_next_state_item(&(stateMsgs[chan]), &stateItem, idx)) {
				stone_id_t itemId = meshStateItemGetId(stateItem);
				if (_advertisedIds.list[i] == itemId) {
					// If eventsOnly is true, only copy the item if it has the event bit set.
					// (this removes all ids without event flag from the recent list)
					if (!eventOnly || meshStateItemIsEventType(stateItem)) {
						tempAdvertisedIds.list[tempAdvertisedIds.size++] = _advertisedIds.list[i];
						found = true;
						break;
					}
				}
			}
		}
		if (!found && i < _advertisedIds.head) {
			// Id at index before head is removed, so decrease head
			_advertisedIds.head--;
		}
	}

	// Then copy the temp list to the recent list
	for (uint8_t i=0; i<tempAdvertisedIds.size; ++i) {
		_advertisedIds.list[i] = tempAdvertisedIds.list[i];
	}
	_advertisedIds.size = tempAdvertisedIds.size;

	// Make sure the head is wrapped around correctly
	if (_advertisedIds.head < 0) {
		_advertisedIds.head = _advertisedIds.size-1;
	}
	if (_advertisedIds.head >= _advertisedIds.size) {
		_advertisedIds.head = 0;
	}
#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("head=%u list:", _advertisedIds.head);
	for (uint8_t i=0; i<_advertisedIds.size; ++i) {
		_logSerial(SERIAL_DEBUG, " %u", _advertisedIds.list[i]);
	}
	_logSerial(SERIAL_DEBUG, SERIAL_CRLF);
#endif
	// Done removing ids from the recent list that are not in the messages


	// Select which id to advertise, start with the head.
	stone_id_t advertiseId = 0;
	if (_advertisedIds.size > 0) {
		advertiseId = _advertisedIds.list[_advertisedIds.head];
	}

	// Add first id that is in the messages, but not in the recent list (search from newest to oldest)
	// If this happens, advertise this id instead of the id at the head
	// Skip ids that are 0 or similar to own id
	// In case of eventOnly: also skip ids that don't have the event flag set
	bool found;
	for (uint8_t chan=0; chan<MESH_STATE_HANDLE_COUNT; ++chan) {
		if (!hasStateMsg[chan]) {
			continue;
		}

		int16_t idx = -1;
		state_item_t* stateItem;
		while (peek_prev_state_item(&(stateMsgs[chan]), &stateItem, idx)) {
			stone_id_t itemId = meshStateItemGetId(stateItem);
			if (itemId == 0 || itemId == ownId) {
				continue;
			}
			if (eventOnly && !meshStateItemIsEventType(stateItem)) {
				continue;
			}
			found = false;
			for (uint8_t i=0; i<_advertisedIds.size; i++) {
				if (_advertisedIds.list[i] == itemId) {
					found = true;
					break;
				}
			}
			if (!found) {
#ifdef PRINT_VERBOSE_EXTERNAL_DATA
				LOGd("found new: %u", itemId);
#endif
				// Add id to recent list
				_advertisedIds.list[_advertisedIds.size] = itemId;
				_advertisedIds.size++;

				// Advertise this id instead of head
				advertiseId = itemId;
				break;
			}
		}
		if (!found) {
			break;
		}
	}
#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("advertiseId=%u", advertiseId);
#endif
	return advertiseId;
}
#endif

#if BUILD_MESHING == 1
void ServiceData::onMeshStateMsg(stone_id_t ownId, state_message_t* stateMsg, uint16_t stateChan) {
	// Check if the last state item is from an event (external data bit is used for that)
	// If so, only advertise states from events for some time
	int16_t idx = -1;
	state_item_t* stateItem;
	while (peek_prev_state_item(stateMsg, &stateItem, idx)) {
		stone_id_t itemId = meshStateItemGetId(stateItem);
		if (meshStateItemIsEventType(stateItem) && itemId != ownId) {
			_numAdvertiseChangedStates = MESH_STATE_HANDLE_COUNT * MAX_STATE_ITEMS;
		}
		onMeshStateSeen(ownId, stateItem, stateChan);
		break;
	}
#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("onMeshStateMsg: %u", _numAdvertiseChangedStates);
#endif
}

void ServiceData::onMeshStateSeen(stone_id_t ownId, state_item_t* stateItem, uint16_t stateChan) {
	stone_id_t itemId = meshStateItemGetId(stateItem);
	if (itemId == ownId) {
		return;
	}

	uint16_t hash = BLEutil::calcHash((uint8_t*)stateItem, sizeof(state_item_t));
	uint32_t currentTime = RTC::getCount();

	// Check if id is already in the last seen table.
	int i;
	for (i=0; i<LAST_SEEN_COUNT_PER_STATE_CHAN; ++i) {
		if (_lastSeenIds[stateChan][i].id == itemId) {
			// Only adjust timestamp when hash isn't similar (aka this state is new)
			if (_lastSeenIds[stateChan][i].hash != hash) {
				_lastSeenIds[stateChan][i].timestamp = currentTime;
				_lastSeenIds[stateChan][i].hash = hash;
				_lastSeenIds[stateChan][i].timedout = 0;
			}
			else {
				LOGd("same hash");
			}
			return;
		}
	}

	// Else, write this id at the first empty spot.
	for (i=0; i<LAST_SEEN_COUNT_PER_STATE_CHAN; ++i) {
		if (_lastSeenIds[stateChan][i].id == 0) {
			_lastSeenIds[stateChan][i].id = itemId;
			_lastSeenIds[stateChan][i].timestamp = currentTime;
			_lastSeenIds[stateChan][i].hash = hash;
			_lastSeenIds[stateChan][i].timedout = 0;
			return;
		}
	}

	// Else, write this id at the spot that is timed out or has the oldest timestamp.
	int oldestIdx = 0;
	uint32_t largestDiff = 0;
	uint32_t diff;
	for (i=0; i<LAST_SEEN_COUNT_PER_STATE_CHAN; ++i) {
		if (_lastSeenIds[stateChan][i].timedout == 1) {
			oldestIdx = i;
			break;
		}
		diff = RTC::difference(currentTime, _lastSeenIds[stateChan][i].timestamp);
		if (diff > largestDiff) {
			largestDiff = diff;
			oldestIdx = i;
		}
	}
	_lastSeenIds[stateChan][oldestIdx].id = itemId;
	_lastSeenIds[stateChan][oldestIdx].timestamp = currentTime;
	_lastSeenIds[stateChan][oldestIdx].hash = hash;
	_lastSeenIds[stateChan][oldestIdx].timedout = 0;
}

bool ServiceData::isMeshStateNotTimedOut(stone_id_t id, uint16_t stateChan, uint32_t currentTime) {
	int numNonTimedOut = 0;
	bool idNonTimedOut = false;
	for (int i=0; i<LAST_SEEN_COUNT_PER_STATE_CHAN; ++i) {
		// Check id
		if (_lastSeenIds[stateChan][i].id == 0) {
			continue;
		}

		if (_lastSeenIds[stateChan][i].timedout == 1) {
			continue;
		}

		uint32_t diff = RTC::difference(currentTime, _lastSeenIds[stateChan][i].timestamp);
//		LOGd("id=%u diff=%u ms=%u ticks=%u", _lastSeenIds[stateChan][i].id, diff, MESH_STATE_TIMEOUT, RTC::msToTicks(MESH_STATE_TIMEOUT));
		if (diff > RTC::msToTicks(MESH_STATE_TIMEOUT)) {
			_lastSeenIds[stateChan][i].timedout = 1;
			continue;
		}
		// Valid id and not timed out
		++numNonTimedOut;
		if (_lastSeenIds[stateChan][i].id == id) {
			idNonTimedOut = true;
		}
	}
//	LOGd("id=%u num=%u", idNonTimedOut, numNonTimedOut);
	return (idNonTimedOut || numNonTimedOut == LAST_SEEN_COUNT_PER_STATE_CHAN);
}
#endif

#if BUILD_MESHING == 1
void ServiceData::_sendMeshState() {
	sendMeshState(_meshNextEventType == 0 ? false : true, _meshNextEventType);
	_meshNextEventType = 0;
}
#endif

#if BUILD_MESHING == 1
void ServiceData::sendMeshState(bool event, uint16_t eventType) {
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {

//		// Update flag
//		updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, Settings::getInstance().isSet(CONFIG_PWM_ALLOWED));
//		updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED));

		uint32_t rtcCount = RTC::getCount();

		// Only send a mesh message if the previous was at least some time ago.
		if (RTC::difference(rtcCount, _meshLastSentTimestamp) < RTC::msToTicks(MESH_STATE_MIN_INTERVAL)) {
			Timer::getInstance().stop(_meshStateTimerId);
			_meshNextEventType = eventType;
			Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(MESH_STATE_MIN_INTERVAL), this);
			return;
		}

		uint32_t timestamp;
		State::getInstance().get(STATE_TIME, timestamp);

		state_item_t stateItem = {};
		if (event) {
			switch (eventType) {
			case STATE_SWITCH_STATE:
			default:
				stateItem.type = MESH_STATE_ITEM_TYPE_EVENT_STATE;
				stateItem.state.id = _crownstoneId;
				stateItem.state.switchState = _switchState;
				stateItem.state.flags = _flags;
				stateItem.state.temperature = _temperature;
				stateItem.state.powerFactor = _powerFactor;
				stateItem.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
				stateItem.state.energyUsed = _energyUsed;
				stateItem.state.partialTimestamp = getPartialTimestampOrCounter(timestamp, _meshSendCount);
				break;
			}
		}
		else {
			stateItem.type = MESH_STATE_ITEM_TYPE_STATE;
			stateItem.state.id = _crownstoneId;
			stateItem.state.switchState = _switchState;
			stateItem.state.flags = _flags;
			stateItem.state.temperature = _temperature;
			stateItem.state.powerFactor = _powerFactor;
			stateItem.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
			stateItem.state.energyUsed = _energyUsed;
			stateItem.state.partialTimestamp = getPartialTimestampOrCounter(timestamp, _meshSendCount);
		}

		MeshControl::getInstance().sendServiceDataMessage(stateItem, event);
		_meshSendCount++;
		_meshLastSentTimestamp = rtcCount;

//		if (!event) {
			Timer::getInstance().stop(_meshStateTimerId);
			// Start timer of MESH_STATE_REFRESH_PERIOD + rand ms
			uint8_t rand8;
			RNG::fillBuffer(&rand8, 1);
			uint32_t randMs = rand8*78; //! Range is 0-19890 ms (about 0-20s)
			Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(MESH_STATE_REFRESH_PERIOD) + MS_TO_TICKS(randMs), this);
//		}
	}
}
#endif

void ServiceData::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	// Keep track of the BLE connection status. If we are connected we do not need to update the packet.
	switch(evt) {
	case EVT_BLE_CONNECT: {
		_connected = true;
		break;
	}
	case EVT_BLE_DISCONNECT: {
		_connected = false;
		updateAdvertisement(false);
		break;
	}
	case EVT_PWM_FORCED_OFF:
	case EVT_SWITCH_FORCED_OFF:
	case EVT_RELAY_FORCED_ON:
//	case EVT_CHIP_TEMP_ABOVE_THRESHOLD:
//	case EVT_PWM_TEMP_ABOVE_THRESHOLD:
		updateFlagsBitmask(SERVICE_DATA_FLAGS_ERROR, true);
		break;
	case STATE_ERRORS: {
		if (length != sizeof(state_errors_t)) {
			break;
		}
		state_errors_t* stateErrors = (state_errors_t*)p_data;
		updateFlagsBitmask(SERVICE_DATA_FLAGS_ERROR, stateErrors->asInt);
		break;
	}
	default: {
		// continue with the rest of the method.
	}
	}

	// In case the operation mode is setup, we have a different advertisement package.
	if (_operationMode == OPERATION_MODE_SETUP) {
		return;
	}
	switch(evt) {
	case CONFIG_CROWNSTONE_ID: {
		updateCrownstoneId(*(uint16_t*)p_data);
		break;
	}
	case STATE_SWITCH_STATE: {
		updateSwitchState(*(uint8_t*)p_data);
#if BUILD_MESHING == 1
		sendMeshState(true, STATE_SWITCH_STATE);
#endif
		break;
	}
	case STATE_ACCUMULATED_ENERGY: {
		int32_t energyUsed = (*(int64_t*)p_data) / 1000 / 1000 / 64;
		updateAccumulatedEnergy(energyUsed);
//		int32_t voltageRms = (*(int32_t*)p_data) / 64; // mV
//		updateAccumulatedEnergy(voltageRms);
//		int32_t currentRms = (*(int32_t*)p_data) * 1000 / 64; // uA
//		updateAccumulatedEnergy(currentRms);
//		int32_t ntcMilliVolt = (*(int32_t*)p_data) * 3000 / 4095 / 64;
//		updateAccumulatedEnergy(ntcMilliVolt);
		// todo create mesh state event if changes significantly
		break;
	}
	case STATE_POWER_USAGE: {
		updatePowerUsage(*(int32_t*)p_data);
		// todo create mesh state event if changes significantly
		break;
	}
	case STATE_TEMPERATURE: {
		// TODO: isn't the temperature an int32_t ?
		updateTemperature(*(int8_t*)p_data);
		break;
	}
	case EVT_TIME_SET: {
		updateFlagsBitmask(SERVICE_DATA_FLAGS_TIME_SET, true);
		break;
	}
	case EVT_PWM_POWERED: {
		updateFlagsBitmask(SERVICE_DATA_FLAGS_DIMMING_AVAILABLE, true);
		break;
	}
	case EVT_PWM_ALLOWED: {
		updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, *(bool*)p_data);
		break;
	}
	case EVT_SWITCH_LOCKED: {
		updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, *(bool*)p_data);
		break;
	}
#if BUILD_MESHING == 1
	case EVT_EXTERNAL_STATE_MSG_CHAN_0: {
		onMeshStateMsg(_crownstoneId, (state_message_t*)p_data, 0);
		break;
	}
	case EVT_EXTERNAL_STATE_MSG_CHAN_1: {
		onMeshStateMsg(_crownstoneId, (state_message_t*)p_data, 1);
		break;
	}
#endif
	case EVT_SWITCHCRAFT_ENABLED: {
		updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED, *(bool*)p_data);
		break;
	}
	// TODO: add bitmask events
	default:
		return;
	}
}




int16_t ServiceData::compressPowerUsageMilliWatt(int32_t powerUsageMW) {
	// units of 1/8 W
	int16_t retVal = powerUsageMW / 125; // similar to *8/1000, but then without chance to overflow
	return retVal;
}

int32_t ServiceData::decompressPowerUsage(int16_t compressedPowerUsage) {
	int32_t retVal = compressedPowerUsage * 125; // similar to /8*1000, but then without losing precision.
	return retVal;
}

int32_t ServiceData::convertEnergyV3ToV1(int32_t energyUsed) {
	int32_t retVal = ((int64_t)energyUsed) * 64 / 3600;
	return retVal;
}

int32_t ServiceData::convertEnergyV1ToV3(int32_t energyUsed) {
	int32_t retVal = ((int64_t)energyUsed) * 3600 / 64;
	return retVal;
}

uint16_t ServiceData::energyToPartialEnergy(int32_t energyUsage) {
//	uint16_t retVal = energyUsage & 0x0000FFFF; // TODO: also drops the sign
	uint16_t retVal = abs(energyUsage) % (UINT16_MAX+1); // Safer
	return retVal;
}

int32_t ServiceData::partialEnergyToEnergy (uint16_t partialEnergy) {
	int32_t retVal = partialEnergy; // TODO: what else can we do?
	return retVal;
}

uint16_t ServiceData::timestampToPartialTimestamp(uint32_t timestamp) {
//	uint16_t retVal = timestamp & 0x0000FFFF;
	uint16_t retVal = timestamp % (UINT16_MAX+1); // Safer
	return retVal;
}

uint16_t ServiceData::getPartialTimestampOrCounter(uint32_t timestamp, uint32_t counter) {
	if (timestamp == 0) {
		return counter;
	}
	return timestampToPartialTimestamp(timestamp);
}
