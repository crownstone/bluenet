/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#include <drivers/cs_Serial.h>
#include <processing/cs_Setup.h>
#include <storage/cs_State.h>

Setup::Setup(): _setupDone(false) {
	EventDispatcher::getInstance().addListener(this);
}

cs_ret_code_t Setup::handleCommand(uint8_t* data, uint16_t size) {
	uint8_t mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, PersistenceMode::STRATEGY1);
	OperationMode _persistenceMode = static_cast<OperationMode>(mode);
	if (_persistenceMode != OperationMode::OPERATION_MODE_SETUP) {
		LOGw("only available in setup mode");
		return ERR_NOT_AVAILABLE;
	}

	if (size != sizeof(setup_data_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	setup_data_t* setupData = (setup_data_t*) data;

	// Validate settings
	if (setupData->type != 0) {
		LOGw("type=%u", setupData->type);
		return ERR_WRONG_PARAMETER;
	}
	if (setupData->id == 0) {
		LOGw("id=0");
		return ERR_WRONG_PARAMETER;
	}
	// TODO: check setupData->meshAccessAddress
	// TODO: check setupData->ibeaconUuid
	if (setupData->ibeaconMajor == 0) {
		LOGw("major=0");
		return ERR_WRONG_PARAMETER;
	}
	if (setupData->ibeaconMinor == 0) {
		LOGw("minor=0");
		return ERR_WRONG_PARAMETER;
	}

	// Save all settings.
	setup_data_t & sd = *setupData;

	State& state = State::getInstance();
	state.set(CS_TYPE::CONFIG_CROWNSTONE_ID, &(sd.id), sizeof(sd.id), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_KEY_ADMIN, sd.adminKey, sizeof(sd.adminKey), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_KEY_MEMBER, sd.memberKey, sizeof(sd.memberKey), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_KEY_GUEST, sd.guestKey, sizeof(sd.guestKey), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_MESH_ACCESS_ADDRESS, &(sd.meshAccessAddress), sizeof(sd.meshAccessAddress), 
			PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_IBEACON_UUID, &(sd.ibeaconUuid.uuid128), sizeof(sd.ibeaconUuid), 
			PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_IBEACON_MAJOR, &(sd.ibeaconMajor), sizeof(sd.ibeaconMajor), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_IBEACON_MINOR, &(sd.ibeaconMinor), sizeof(sd.ibeaconMinor), PersistenceMode::STRATEGY1);

	// Set operation mode to normal mode
	mode = +OperationMode::OPERATION_MODE_NORMAL;
	state.set(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode), PersistenceMode::STRATEGY1);

	// Switch relay on
	event_t event(CS_TYPE::EVT_POWER_ON);
	EventDispatcher::getInstance().dispatch(event);

	// Reboot will be done when persistent storage is done.
	_setupDone = true;

	LOGi("Setup completed");
	return ERR_WAIT_FOR_SUCCESS;
}

void Setup::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_STORAGE_WRITE_DONE:
			if (_setupDone) {
				// set char value
				event_t event1(CS_TYPE::EVT_SETUP_DONE);
				EventDispatcher::getInstance().dispatch(event1);

				// reset after 1000 ms
				evt_do_reset_delayed_t payload;
				payload.resetCode = GPREGRET_SOFT_RESET;
				payload.delayMs = 1000;
				event_t event2(CS_TYPE::EVT_DO_RESET_DELAYED, &payload, sizeof(payload));
				EventDispatcher::getInstance().dispatch(event2);
			}
			break;
		default: {}
	}
}

