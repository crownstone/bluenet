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

ServiceData::ServiceData() : EventListener(EVT_ALL)
{
	EventDispatcher::getInstance().addListener(this);
	memset(_array, 0, sizeof(_array));
	_params.protocolVersion = SERVICE_DATA_PROTOCOL_VERSION;
};

void ServiceData::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
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
		default:
			return;
		}

		// encrypt the array using the guest key ECB if encryption is enabled.
		uint8_t opMode;
		State::getInstance().get(STATE_OPERATION_MODE, opMode);
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED) && !(opMode == OPERATION_MODE_SETUP)) {
			EncryptionHandler::getInstance().encryptAdvertisement(_array, sizeof(_array), _encryptedArray, sizeof(_encryptedArray));
		}

		EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_UPDATED);
	}
