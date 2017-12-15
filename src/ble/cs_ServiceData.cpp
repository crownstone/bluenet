/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#include <ble/cs_ServiceData.h>

#include <processing/cs_EncryptionHandler.h>

#include <protocol/cs_StateTypes.h>
#include <protocol/cs_ConfigTypes.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_RNG.h>
#include <storage/cs_State.h>
#include <util/cs_Utils.h>
#include <drivers/cs_RTC.h>

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
#include <protocol/mesh/cs_MeshMessageState.h>
#endif

#define ADVERTISE_EXTERNAL_DATA
#define PRINT_DEBUG_EXTERNAL_DATA
//#define PRINT_VERBOSE_EXTERNAL_DATA

ServiceData::ServiceData() : _updateTimerId(NULL), _connected(false)
{
	// we want to update the advertisement packet on a fixed interval.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	// get the operation mode from state
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

	EventDispatcher::getInstance().addListener(this);

	// Initialize the service data
	memset(_serviceData.array, 0, sizeof(_serviceData.array));
	_serviceData.params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
	memset(_serviceDataExt.array, 0, sizeof(_serviceDataExt.array));
	_serviceDataExt.params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
	_encryptedParams.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION; // this part will not be written over

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

	_updateCount = 0;

	// set the initial advertisement.
	updateAdvertisement(true);
};

void ServiceData::updateAdvertisement(bool initial) {
	Timer::getInstance().stop(_updateTimerId);

	// if we are connected, we do not need to update the packet.
	if (_connected == false) {
		_updateCount++;

		// in case the operation mode is setup, we have a different advertisement package.
		updateEventBitmask(SETUP_MODE_ENABLED, _operationMode == OPERATION_MODE_SETUP);

		service_data_t* serviceData = &_serviceData;

		// Every 2 updates, we advertise the state of another crownstone.
		if (_updateCount % 2 == 0) {
			if (getExternalAdvertisement(_serviceData.params.crownstoneId, _serviceDataExt)) {
				serviceData = &_serviceDataExt;
			}
		}

		// We use one random number (only if encrypted) and a uint16 counter value for the last 2 bytes.
		// Counter is cheaper than random.
		serviceData->params.counter += 1;

		// encrypt the array using the guest key ECB if encryption is enabled.
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED)) {

			// populate the random field.
			RNG::fillBuffer(&(serviceData->params.rand), 1);

//			BLEutil::printArray(serviceData->array, sizeof(service_data_t));

			// encrypt the block.
			EncryptionHandler::getInstance().encrypt((serviceData->array) + 1, sizeof(service_data_t) - 1, _encryptedParams.payload,
			                                         sizeof(_encryptedParams.payload), GUEST, ECB_GUEST);
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

bool ServiceData::getExternalAdvertisement(uint16_t ownId, service_data_t& serviceData) {
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

	id_type_t advertiseId = 0;

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
		state_item_t* p_stateItem;
		while (peek_prev_state_item(&(messages[chan]), &p_stateItem, idx)) {
			if (p_stateItem->id == advertiseId) {
				if (!isMeshStateNotTimedOut(advertiseId, chan, currentTime)) {
					advertise = false;
				}
				serviceData.params.crownstoneId = p_stateItem->id;
				serviceData.params.switchState = p_stateItem->switchState;
				serviceData.params.eventBitmask = p_stateItem->eventBitmask;
				serviceData.params.eventBitmask |= 1 << SHOWING_EXTERNAL_DATA;
				serviceData.params.temperature = 0; // Unknown
				serviceData.params.powerUsage = p_stateItem->powerUsageApparent * 1000 / 16;
				serviceData.params.accumulatedEnergy = p_stateItem->accumulatedEnergy;
				found = true;
				break;
			}
		}
		if (found) {
			break;
		}
	}

	// If we advertised the id at head, then increase the head
	if (advertiseId == _advertisedIds.list[_advertisedIds.head]) {
		_advertisedIds.head = (_advertisedIds.head + 1) % _advertisedIds.size;
	}

#ifdef PRINT_DEBUG_EXTERNAL_DATA
	LOGd("serviceData: id=%u switch=%u bitmask=%u temp=%i P=%i E=%i adv=%u", serviceData.params.crownstoneId, serviceData.params.switchState, serviceData.params.eventBitmask, serviceData.params.temperature, serviceData.params.powerUsage, serviceData.params.accumulatedEnergy, advertise);
#endif

	return advertise;
#else
	return false;
#endif
}

#if BUILD_MESHING == 1
id_type_t ServiceData::chooseExternalId(uint16_t ownId, state_message_t stateMsgs[], bool hasStateMsg[], bool eventOnly) {

#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("chooseExternalId");
	LOGd("head=%u list:", _advertisedIds.head);
	for (uint8_t i=0; i<_advertisedIds.size; ++i) {
		_log(SERIAL_DEBUG, " %u", _advertisedIds.list[i]);
	}
	_log(SERIAL_DEBUG, SERIAL_CRLF);

	LOGd("msgs:");
	for (uint8_t chan=0; chan<MESH_STATE_HANDLE_COUNT; ++chan) {
		if (!hasStateMsg[chan]) {
			continue;
		}
		int16_t idx = -1;
		state_item_t* p_stateItem;
		while (peek_next_state_item(&(stateMsgs[chan]), &p_stateItem, idx)) {
			_log(SERIAL_DEBUG, "  id=%u mask=%u", p_stateItem->id, p_stateItem->eventBitmask);
		}
	}
	_log(SERIAL_DEBUG, SERIAL_CRLF);
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
			state_item_t* p_stateItem;
			while (peek_next_state_item(&(stateMsgs[chan]), &p_stateItem, idx)) {
				if (_advertisedIds.list[i] == p_stateItem->id) {
					// If eventsOnly is true, only copy the item if it has the event bit set.
					// (this removes all ids without event flag from the recent list)
					if (!eventOnly || BLEutil::isBitSet(p_stateItem->eventBitmask, SHOWING_EXTERNAL_DATA)) {
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
		_log(SERIAL_DEBUG, " %u", _advertisedIds.list[i]);
	}
	_log(SERIAL_DEBUG, SERIAL_CRLF);
#endif
	// Done removing ids from the recent list that are not in the messages


	// Select which id to advertise, start with the head.
	id_type_t advertiseId = 0;
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
		state_item_t* p_stateItem;
		while (peek_prev_state_item(&(stateMsgs[chan]), &p_stateItem, idx)) {
			if (p_stateItem->id == 0 || p_stateItem->id == ownId) {
				continue;
			}
			if (eventOnly && !BLEutil::isBitSet(p_stateItem->eventBitmask, SHOWING_EXTERNAL_DATA)) {
				continue;
			}
			found = false;
			for (uint8_t i=0; i<_advertisedIds.size; i++) {
				if (_advertisedIds.list[i] == p_stateItem->id) {
					found = true;
					break;
				}
			}
			if (!found) {
#ifdef PRINT_VERBOSE_EXTERNAL_DATA
				LOGd("found new: %u", p_stateItem->id);
#endif
				// Add id to recent list
				_advertisedIds.list[_advertisedIds.size] = p_stateItem->id;
				_advertisedIds.size++;

				// Advertise this id instead of head
				advertiseId = p_stateItem->id;
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
void ServiceData::onMeshStateMsg(id_type_t ownId, state_message_t* stateMsg, uint16_t stateChan) {
	// Check if the last state item is from an event (external data bit is used for that)
	// If so, only advertise states from events for some time
	int16_t idx = -1;
	state_item_t* p_stateItem;
	while (peek_prev_state_item(stateMsg, &p_stateItem, idx)) {
		if (BLEutil::isBitSet(p_stateItem->eventBitmask, SHOWING_EXTERNAL_DATA) && p_stateItem->id != ownId) {
			_numAdvertiseChangedStates = MESH_STATE_HANDLE_COUNT * MAX_STATE_ITEMS;
		}
		onMeshStateSeen(ownId, p_stateItem, stateChan);
		break;
	}
#ifdef PRINT_VERBOSE_EXTERNAL_DATA
	LOGd("onMeshStateMsg: %u", _numAdvertiseChangedStates);
#endif
}

void ServiceData::onMeshStateSeen(id_type_t ownId, state_item_t* stateItem, uint16_t stateChan) {
	if (stateItem->id == ownId) {
		return;
	}

	uint16_t hash = BLEutil::calcHash((uint8_t*)stateItem, sizeof(state_item_t));
	uint32_t currentTime = RTC::getCount();

	// Check if id is already in the last seen table.
	int i;
	for (i=0; i<LAST_SEEN_COUNT_PER_STATE_CHAN; ++i) {
		if (_lastSeenIds[stateChan][i].id == stateItem->id) {
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
			_lastSeenIds[stateChan][i].id = stateItem->id;
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
	_lastSeenIds[stateChan][oldestIdx].id = stateItem->id;
	_lastSeenIds[stateChan][oldestIdx].timestamp = currentTime;
	_lastSeenIds[stateChan][oldestIdx].hash = hash;
	_lastSeenIds[stateChan][oldestIdx].timedout = 0;
}

bool ServiceData::isMeshStateNotTimedOut(id_type_t id, uint16_t stateChan, uint32_t currentTime) {
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

void ServiceData::sendMeshState(bool event) {
#if BUILD_MESHING == 1
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {

		state_item_t stateItem = {};
		stateItem.id = _serviceData.params.crownstoneId;
		stateItem.switchState = _serviceData.params.switchState;
		stateItem.eventBitmask = _serviceData.params.eventBitmask;
		if (event) {
			// Use the "external data" bit to flag this data as coming from an event.
			stateItem.eventBitmask |= 1 << SHOWING_EXTERNAL_DATA;
		}
		else {
			// Although probably not necessary, remove the "external data" bit
			stateItem.eventBitmask &= ~(1 << SHOWING_EXTERNAL_DATA);
		}
		// Translate advertisement v1 to advertisement v2
		stateItem.powerFactor = 1024; // Assume a power factor of 1
		stateItem.powerUsageApparent = 0;
		if (_serviceData.params.powerUsage > 0) {
			stateItem.powerUsageApparent = _serviceData.params.powerUsage * 16 / 1000;
		}
		stateItem.accumulatedEnergy = _serviceData.params.accumulatedEnergy;

		MeshControl::getInstance().sendServiceDataMessage(stateItem, event);

		if (!event) {
			Timer::getInstance().stop(_meshStateTimerId);
			// Start timer of MESH_STATE_REFRESH_PERIOD + rand ms
			uint8_t rand8;
			RNG::fillBuffer(&rand8, 1);
			uint32_t randMs = rand8*78; //! Range is 0-19890 ms (about 0-20s)
			Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(MESH_STATE_REFRESH_PERIOD) + MS_TO_TICKS(randMs), this);
		}
	}
#endif
}

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
//	case EVT_CHIP_TEMP_ABOVE_THRESHOLD:
//	case EVT_PWM_TEMP_ABOVE_THRESHOLD:
		updateEventBitmask(SERVICE_BITMASK_ERROR, true);
		break;
	case STATE_ERRORS: {
		if (length != sizeof(state_errors_t)) {
			break;
		}
		state_errors_t* stateErrors = (state_errors_t*)p_data;
		updateEventBitmask(SERVICE_BITMASK_ERROR, stateErrors->asInt);
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
		sendMeshState(true);
		break;
	}
	case STATE_ACCUMULATED_ENERGY: {
		updateAccumulatedEnergy(*(int32_t*)p_data);
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
#if BUILD_MESHING == 1
	case EVT_EXTERNAL_STATE_MSG_CHAN_0: {
		onMeshStateMsg(_serviceData.params.crownstoneId, (state_message_t*)p_data, 0);
		break;
	}
	case EVT_EXTERNAL_STATE_MSG_CHAN_1: {
		onMeshStateMsg(_serviceData.params.crownstoneId, (state_message_t*)p_data, 1);
		break;
	}
#endif
	// TODO: add bitmask events
	default:
		return;
	}
}
