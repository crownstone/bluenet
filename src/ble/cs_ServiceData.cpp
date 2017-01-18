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
#include <protocol/cs_MeshMessageTypes.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_RNG.h>
#include <mesh/cs_MeshControl.h>

ServiceData::ServiceData() : EventListener(EVT_ALL), _updateTimerId(NULL), _connected(false), _lastStateChangeMessage({})
{
	//! we want to update the advertisement packet every 1 second.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	_meshStateTimerData = { {0} };
	_meshStateTimerId = &_meshStateTimerData;
	Timer::getInstance().createSingleShot(_meshStateTimerId, (app_timer_timeout_handler_t)ServiceData::meshStateTick);

	//! get the OP mode from state
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

	EventDispatcher::getInstance().addListener(this);
	memset(_array, 0, sizeof(_array));
	_params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
	_encryptedParams.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION; // this part will not be written over

	//! start the update timer
	Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);

	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED) && _operationMode != OPERATION_MODE_SETUP) {
		//! start the mesh state timer
		Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(MESH_STATE_REFRESH_PERIOD), this);
	}

	//! set the initial advertisement.
	updateAdvertisement();
};

void ServiceData::updateAdvertisement() {
	Timer::getInstance().stop(_updateTimerId);

	//! if we are connected, we do not need to update the packet.
	if (_connected == false) {

		//! in case the operation mode is setup, we have a different advertisement package.
		updateEventBitmask(SETUP_MODE_ENABLED, _operationMode == OPERATION_MODE_SETUP);

		//! We use one random number (only if encrypted) and a uint16 counter value for the last 2 bytes.
		//! Counter is cheaper than random.
		_params.counter += 1;

		//! encrypt the array using the guest key ECB if encryption is enabled.
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED)) {

			//! populate the random field.
			RNG::fillBuffer(&_params.rand, 1);

			//! encrypt the block.
			EncryptionHandler::getInstance().encrypt(_array + 1, sizeof(_array) - 1, _encryptedParams.payload,
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
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {

		state_item_t stateItem = {};
		stateItem.id = _params.crownstoneId;
		stateItem.switchState = _params.switchState;
		stateItem.powerUsage = _params.powerUsage;
		stateItem.accumulatedEnergy = _params.accumulatedEnergy;

		MeshControl::getInstance().sendServiceDataMessage(stateItem, event);

		if (!event) {
			Timer::getInstance().start(_meshStateTimerId, MS_TO_TICKS(MESH_STATE_REFRESH_PERIOD), this);
		}
	}
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
			break;
		}
		case STATE_POWER_USAGE: {
			updatePowerUsage(*(int32_t*)p_data);
			break;
		}
		case STATE_TEMPERATURE: {
			// TODO: isn't the temperature an int32_t ?
			updateTemperature(*(int8_t*)p_data);
			break;
		}
		//! TODO: add bitmask events
		default:
			return;
		}
	}
}
