/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <processing/cs_Setup.h>

Setup::Setup() {
	EventDispatcher::getInstance().addListener(this);
}

cs_ret_code_t Setup::handleCommand(uint8_t* data, uint16_t size) {
	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	OperationMode operationMode = getOperationMode(mode);

	if (operationMode != OperationMode::OPERATION_MODE_SETUP) {
		LOGw("only available in setup mode");
		return ERR_NOT_AVAILABLE;
	}

	if (size != sizeof(setup_data_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	setup_data_t* setupData = (setup_data_t*) data;

	// Validate settings
	if (setupData->stoneId == 0) {
		LOGw("stoneId=0");
		return ERR_WRONG_PARAMETER;
	}
	if (setupData->sphereId == 0) {
		LOGw("sphereId=0");
		return ERR_WRONG_PARAMETER;
	}

	LOGd("Store keys, mesh address, and other config data");
	State &state = State::getInstance();
	state.set(CS_TYPE::CONFIG_CROWNSTONE_ID,    &(setupData->stoneId), sizeof(setupData->stoneId));
	state.set(CS_TYPE::CONFIG_SPHERE_ID,        &(setupData->sphereId), sizeof(setupData->sphereId));
	state.set(CS_TYPE::CONFIG_KEY_ADMIN,        &(setupData->adminKey), sizeof(setupData->adminKey));
	state.set(CS_TYPE::CONFIG_KEY_MEMBER,       &(setupData->memberKey), sizeof(setupData->memberKey));
	state.set(CS_TYPE::CONFIG_KEY_BASIC,        &(setupData->basicKey), sizeof(setupData->basicKey));
	state.set(CS_TYPE::CONFIG_KEY_SERVICE_DATA, &(setupData->serviceDataKey), sizeof(setupData->serviceDataKey));
	state.set(CS_TYPE::CONFIG_MESH_DEVICE_KEY,  &(setupData->meshDeviceKey), sizeof(setupData->meshDeviceKey));
	state.set(CS_TYPE::CONFIG_MESH_APP_KEY,     &(setupData->meshAppKey), sizeof(setupData->meshAppKey));
	state.set(CS_TYPE::CONFIG_MESH_NET_KEY,     &(setupData->meshNetKey), sizeof(setupData->meshNetKey));
	state.set(CS_TYPE::CONFIG_IBEACON_UUID,     &(setupData->ibeaconUuid.uuid128), sizeof(setupData->ibeaconUuid.uuid128));
	state.set(CS_TYPE::CONFIG_IBEACON_MAJOR,    &(setupData->ibeaconMajor), sizeof(setupData->ibeaconMajor));
	state.set(CS_TYPE::CONFIG_IBEACON_MINOR,    &(setupData->ibeaconMinor), sizeof(setupData->ibeaconMinor));

	// Set operation mode to normal mode
	operationMode = OperationMode::OPERATION_MODE_NORMAL;
	mode = to_underlying_type(operationMode);
	LOGi("Set mode NORMAL: 0x%X", mode);
	state.set(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));

	// Switch relay on
	event_t event0(CS_TYPE::CMD_SWITCH_ON);
	EventDispatcher::getInstance().dispatch(event0);

	// TODO: keep up if all configs are successfully stored, maybe with a bitmask.
	_successfullyStoredBitmask = 0;

	LOGi("Setup completed");
	return ERR_WAIT_FOR_SUCCESS;
}

/**
 * This event handler only listens to one event, namely EVT_STORAGE_WRITE_DONE. In that case it will continue with
 * a reset procedure. However, how does it know that the right item has been written? What should be done instead is
 * just calling a reset procedure that - in a loop - queries if there are peripherals still busy. As soon as for
 * example the entire storage queue has been handled, it should perform the reset.
 */
void Setup::handleEvent(event_t & event) {
	// we want to react to the last write request (OPERATION_MODE_NORMAL)
	if (event.type != CS_TYPE::EVT_ADVERTISEMENT_UPDATED) {
		LOGnone("Setup received %s [%i]", TypeName(event.type), to_underlying_type(event.type));
	}
	switch (event.type) {
	case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
		CS_TYPE storedType = *(CS_TYPE*)event.data;
		onStorageDone(storedType);
		break;
	}
	default: {
		break;
	}
	}
}

void Setup::onStorageDone(const CS_TYPE& type) {
	LOGd("storage done %u", to_underlying_type(type));
	switch (type) {
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_STONE_ID);
		break;
	case CS_TYPE::CONFIG_SPHERE_ID:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_SPHERE_ID);
		break;
	case CS_TYPE::CONFIG_KEY_ADMIN:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_ADMIN_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_MEMBER:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MEMBER_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_BASIC:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_BASIC_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_SERVICE_DATA:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_SERVICE_DATA_KEY);
		break;
	case CS_TYPE::CONFIG_MESH_DEVICE_KEY:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MESH_DEVICE_KEY);
		break;
	case CS_TYPE::CONFIG_MESH_APP_KEY:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MESH_APP_KEY);
		break;
	case CS_TYPE::CONFIG_MESH_NET_KEY:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MESH_NET_KEY);
		break;
	case CS_TYPE::CONFIG_IBEACON_UUID:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_IBEACON_UUID);
		break;
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_IBEACON_MAJOR);
		break;
	case CS_TYPE::CONFIG_IBEACON_MINOR:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_IBEACON_MINOR);
		break;
	case CS_TYPE::STATE_SWITCH_STATE:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_SWITCH);
		break;
	case CS_TYPE::STATE_OPERATION_MODE:
		BLEutil::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_OPERATION_MODE);
		break;
	default:
		break;
	}
	if ((_successfullyStoredBitmask & SETUP_CONFIG_MASK_ALL) == SETUP_CONFIG_MASK_ALL) {
		LOGi("All state variables stored");
		finalize();
	}
}

void Setup::finalize() {
	LOGi("Setup done... Reset crownstone");
	// set char value
	event_t event1(CS_TYPE::EVT_SETUP_DONE);
	EventDispatcher::getInstance().dispatch(event1);

	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	LOGd("New mode is 0x%X", mode);
	OperationMode operationMode = static_cast<OperationMode>(mode);
	LOGd("Operation mode: %s", TypeName(operationMode));
	if (!ValidMode(operationMode)) {
		LOGe("Invalid operation mode!");
		// for now continue with reset (will be considered setup mode)
	}

	// reset after 1000 ms
	reset_delayed_t resetDelayed;
	resetDelayed.resetCode = GPREGRET_SOFT_RESET;
	resetDelayed.delayMs = 1000;
	event_t event2(CS_TYPE::CMD_RESET_DELAYED, &resetDelayed, sizeof(resetDelayed));
	EventDispatcher::getInstance().dispatch(event2);
}

