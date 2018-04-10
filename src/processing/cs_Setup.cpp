/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#include "processing/cs_Setup.h"
#include "drivers/cs_Serial.h"
#include "storage/cs_Settings.h"
#include "storage/cs_State.h"

Setup::Setup(): _setupDone(false) {
	EventDispatcher::getInstance().addListener(this);
}

ERR_CODE Setup::handleCommand(uint8_t* data, uint16_t size) {
	uint8_t opMode;
	State::getInstance().get(STATE_OPERATION_MODE, opMode);
	if (opMode != OPERATION_MODE_SETUP) {
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
	// Since all settings are at the same flash page, we store to persistent storage at the end.
	Settings& settings = Settings::getInstance();
	settings.set(CONFIG_CROWNSTONE_ID,       &(setupData->id),                  false, sizeof(setupData->id));
	settings.set(CONFIG_KEY_ADMIN,           setupData->adminKey,               false, sizeof(setupData->adminKey));
	settings.set(CONFIG_KEY_MEMBER,          setupData->memberKey,              false, sizeof(setupData->memberKey));
	settings.set(CONFIG_KEY_GUEST,           setupData->guestKey,               false, sizeof(setupData->guestKey));
	settings.set(CONFIG_MESH_ACCESS_ADDRESS, &(setupData->meshAccessAddress),   false, sizeof(setupData->meshAccessAddress));
	settings.set(CONFIG_IBEACON_UUID,        &(setupData->ibeaconUuid.uuid128), false, sizeof(setupData->ibeaconUuid));
	settings.set(CONFIG_IBEACON_MAJOR,       &(setupData->ibeaconMajor),        false, sizeof(setupData->ibeaconMajor));
	settings.set(CONFIG_IBEACON_MINOR,       &(setupData->ibeaconMinor),        false, sizeof(setupData->ibeaconMinor));
	settings.savePersistentStorage();

	// Set operation mode to normal mode
	State::getInstance().set(STATE_OPERATION_MODE, (uint8_t)OPERATION_MODE_NORMAL);

	// Switch relay on
	EventDispatcher::getInstance().dispatch(EVT_POWER_ON);

	// Reboot will be done when persistent storage is done.
	_setupDone = true;

	LOGi("Setup completed");
	return ERR_WAIT_FOR_SUCCESS;
}

void Setup::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_STORAGE_DONE:
		if (_setupDone) {
			// set char value
			EventDispatcher::getInstance().dispatch(EVT_SETUP_DONE);

			// reset after X seconds
			uint8_t opCode = GPREGRET_SOFT_RESET;
			EventDispatcher::getInstance().dispatch(EVT_DO_RESET_DELAYED, &opCode, sizeof(opCode));
		}
		break;
	}
}

