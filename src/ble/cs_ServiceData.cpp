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

ServiceData::ServiceData() : EventListener(EVT_ALL)
{
	//! get the OP mode from state
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

	EventDispatcher::getInstance().addListener(this);
	memset(_array, 0, sizeof(_array));
	_params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
	_encryptedParams.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION; // this part will not be written over

	//! in case the operation mode is setup, we have a different advertisement package.
	if (_operationMode == OPERATION_MODE_SETUP) {
		updateEventBitmask(SETUP_MODE_ENABLED, true);
	}
	else {
		//! set bitmask to NOT SETUP MODE
		updateEventBitmask(SETUP_MODE_ENABLED,false);
	}
};

void ServiceData::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
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

		//! set bitmask to NOT SETUP MODE
		updateEventBitmask(7,false);

		//! encrypt the array using the guest key ECB if encryption is enabled.
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED)) {
			//! populate the random field. We use one random number and a uint16 couter value for the last 2 bytes. Counter is cheaper than random.
			RNG::fillBuffer(&_params.rand, 1);
			_params.counter += 1;

			//! encrypt the block.
			EncryptionHandler::getInstance().encrypt(_array+1, sizeof(_array)-1, _encryptedParams.payload, sizeof(_encryptedParams.payload), GUEST, ECB_GUEST);
		}

		EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_UPDATED);
	}
}
