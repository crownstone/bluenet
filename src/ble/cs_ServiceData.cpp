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

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
#endif

ServiceData::ServiceData() : EventListener(EVT_ALL), _updateTimerId(NULL), _connected(false)
{
	//! we want to update the advertisement packet on a fixed interval.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	//! get the operation mode from state
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

	EventDispatcher::getInstance().addListener(this);

	//! Initialize the service data
	memset(_serviceData.array, 0, sizeof(_serviceData.array));
	_serviceData.params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
	memset(_serviceDataExt.array, 0, sizeof(_serviceDataExt.array));
	_serviceDataExt.params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
	_encryptedParams.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION; // this part will not be written over

	//! start the update timer
	Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);

#if BUILD_MESHING == 1
	_lastStateChangeMessage = {};

	_meshStateTimerData = { {0} };
	_meshStateTimerId = &_meshStateTimerData;
	Timer::getInstance().createSingleShot(_meshStateTimerId, (app_timer_timeout_handler_t)ServiceData::meshStateTick);

	//! Only send the state over the mesh in normal mode
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED) && _operationMode == OPERATION_MODE_NORMAL) {
		//! start the mesh state timer with a small random delay
		uint8_t rand8;
		RNG::fillBuffer(&rand8, 1);
		uint32_t randMs = rand8*10;
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
	//! set the initial advertisement.
	updateAdvertisement();
};

void ServiceData::updateAdvertisement() {
	Timer::getInstance().stop(_updateTimerId);

	//! if we are connected, we do not need to update the packet.
	if (_connected == false) {
		_updateCount++;

		//! in case the operation mode is setup, we have a different advertisement package.
		updateEventBitmask(SETUP_MODE_ENABLED, _operationMode == OPERATION_MODE_SETUP);

		service_data_t* serviceData = &_serviceData;

#if BUILD_MESHING == 1
		//! Every N updates, we advertise the state of another crownstone.
		if (_operationMode == OPERATION_MODE_NORMAL && _updateCount % 2 == 0 && Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
			state_message_t message = {};
			uint16_t messageSize = sizeof(state_message_t);

			if (_numAdvertiseChangedStates > 0) {
				MeshControl::getInstance().getLastStateDataMessage(message, messageSize, true);
				_numAdvertiseChangedStates--;
			}
			else {
				MeshControl::getInstance().getLastStateDataMessage(message, messageSize, false);
			}
			if (message.size) {
				int16_t idx = -1;
				state_item_t* p_stateItem;

				//! Remove ids that are in the list, but not in the message
				_tempAdvertisedIds.size = 0;
				for (auto i=0; i<_advertisedIds.size; i++) {
					bool found = false;
					idx = -1;
					while (peek_next_state_item(&message, &p_stateItem, idx)) {
						if (_advertisedIds.list[i] == p_stateItem->id) {
							//! First copy the ones that are in both to a temp list
							_tempAdvertisedIds.list[_tempAdvertisedIds.size++] = _advertisedIds.list[i];
							found = true;
							break;
						}
					}
					if (!found && i < _advertisedIds.head) {
						//! Id at index before head got removed, so decrease head
						_advertisedIds.head--;
					}
				}
				//! Then copy the temp list to the list
				for (auto i=0; i<_tempAdvertisedIds.size; i++) {
					_advertisedIds.list[i] = _tempAdvertisedIds.list[i];
				}
				_advertisedIds.size = _tempAdvertisedIds.size;

				//! Make sure the head is wrapped around correctly
				if (_advertisedIds.head < 0) {
					_advertisedIds.head = _advertisedIds.size-1;
				}
				if (_advertisedIds.head >= _advertisedIds.size) {
					_advertisedIds.head = 0;
				}

				uint8_t advertiseId = 0;
				if (_advertisedIds.size > 0) {
					advertiseId = _advertisedIds.list[_advertisedIds.head];
				}

				//! Add first id that is in the message, but not in the list (search from newest to oldest)
				//! If this happens, advertise this id instead of the id at the head
				//! Skip ids that are 0 or similar to own id
				idx = -1;
				while (peek_prev_state_item(&message, &p_stateItem, idx)) {
					if (p_stateItem->id == 0 || p_stateItem->id == _serviceData.params.crownstoneId) {
						continue;
					}
					bool found = false;
					for (auto i=0; i<_advertisedIds.size; i++) {
						if (_advertisedIds.list[i] == p_stateItem->id) {
							found = true;
							break;
						}
					}
					if (!found) {
						//! Add item to list
						_advertisedIds.list[_advertisedIds.size] = p_stateItem->id;
						_advertisedIds.size++;

						//! Advertise this id instead of head
						advertiseId = p_stateItem->id;
						break;
					}
				}

				//! Advertise the selected id (if it's not 0)
				if (advertiseId != 0) {
					idx = -1;
					while (peek_prev_state_item(&message, &p_stateItem, idx)) {
						if (p_stateItem->id == advertiseId) {
//							LOGd("Advertise external data");
							serviceData = &_serviceDataExt;
							serviceData->params.crownstoneId = p_stateItem->id;
							serviceData->params.switchState = p_stateItem->switchState;
							serviceData->params.eventBitmask = p_stateItem->eventBitmask;
							serviceData->params.eventBitmask |= 1 << SHOWING_EXTERNAL_DATA;
							serviceData->params.powerUsage = p_stateItem->powerUsage;
							serviceData->params.accumulatedEnergy = p_stateItem->accumulatedEnergy;
							break;
						}
					}

					//! If we advertised the id at head, then increase the head
					if (advertiseId == _advertisedIds.list[_advertisedIds.head]) {
						_advertisedIds.head = (_advertisedIds.head + 1) % _advertisedIds.size;
					}
				}
			}
		}
#endif

		//! We use one random number (only if encrypted) and a uint16 counter value for the last 2 bytes.
		//! Counter is cheaper than random.
		serviceData->params.counter += 1;

		//! encrypt the array using the guest key ECB if encryption is enabled.
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED)) {

			//! populate the random field.
			RNG::fillBuffer(&(serviceData->params.rand), 1);

//			BLEutil::printArray(serviceData->array, sizeof(service_data_t));

			//! encrypt the block.
			EncryptionHandler::getInstance().encrypt((serviceData->array) + 1, sizeof(service_data_t) - 1, _encryptedParams.payload,
			                                         sizeof(_encryptedParams.payload), GUEST, ECB_GUEST);
		}

		EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_UPDATED);
	}

	//! start the timer again.
	if (_operationMode == OPERATION_MODE_SETUP) {
		Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD_SETUP), this);
	}
	else {
		Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);
	}
}

void ServiceData::sendMeshState(bool event) {
#if BUILD_MESHING == 1
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {

		state_item_t stateItem = {};
		stateItem.id = _serviceData.params.crownstoneId;
		stateItem.switchState = _serviceData.params.switchState;
		stateItem.eventBitmask = _serviceData.params.eventBitmask;
		stateItem.powerUsage = _serviceData.params.powerUsage;
		stateItem.accumulatedEnergy = _serviceData.params.accumulatedEnergy;

		MeshControl::getInstance().sendServiceDataMessage(stateItem, event);

		if (!event) {
			//! Start timer of MESH_STATE_REFRESH_PERIOD + rand(0,255)*10 ms
			uint8_t rand8;
			RNG::fillBuffer(&rand8, 1);
			uint32_t randMs = rand8*10;
			Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(MESH_STATE_REFRESH_PERIOD) + MS_TO_TICKS(randMs), this);
		}
	}
#endif
}

void ServiceData::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	//! keep track of the BLE connection status. If we are connected we do not need to update the packet.
	switch(evt) {
	case EVT_BLE_CONNECT: {
			_connected = true;
			break;
		}
		case EVT_BLE_DISCONNECT: {
			_connected = false;
			updateAdvertisement();
			break;
		}
		case EVT_PWM_FORCED_OFF:
		case EVT_SWITCH_FORCED_OFF:
			updateEventBitmask(SERVICE_BITMASK_ERROR, true);
			break;
		default: {
			//! continue with the rest of the method.
		}
	}

	//! in case the operation mode is setup, we have a different advertisement package.
	if (_operationMode == OPERATION_MODE_SETUP) {
		return;
	}
	else {
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
		case EVT_EXTERNAL_STATE_CHANGE: {
#if BUILD_MESHING == 1
			_numAdvertiseChangedStates = MAX_STATE_ITEMS;
#endif
			break;
		}
		//! TODO: add bitmask events
		default:
			return;
		}
	}
}
