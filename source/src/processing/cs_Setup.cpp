/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <processing/cs_Setup.h>
#include <storage/cs_State.h>
#include <util/cs_Utils.h>


#define LOGSetupDebug LOGd
#define LOGSetupInfo LOGi

Setup::Setup() {
	listen();
}

cs_ret_code_t Setup::handleCommand(cs_data_t data) {
	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	OperationMode operationMode = getOperationMode(mode);

	if (operationMode != OperationMode::OPERATION_MODE_SETUP) {
		LOGw("only available in setup mode");
		return ERR_NOT_AVAILABLE;
	}

	if (data.len != sizeof(setup_data_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, data.len, sizeof(setup_data_t));
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	setup_data_t* setupData = (setup_data_t*) data.data;

	// Validate settings
	if (setupData->stoneId == 0) {
		LOGw("Invalid stoneId=0");
		return ERR_WRONG_PARAMETER;
	}
	if (setupData->sphereId == 0) {
		LOGw("Invalid sphereId=0");
		return ERR_WRONG_PARAMETER;
	}

	// Reset which types were successfully stored.
	_successfullyStoredBitmask = 0;

	LOGd("Store keys, mesh address, and other config data");
	setWithCheck(CS_TYPE::CONFIG_CROWNSTONE_ID,    &(setupData->stoneId), sizeof(setupData->stoneId));
	setWithCheck(CS_TYPE::CONFIG_SPHERE_ID,        &(setupData->sphereId), sizeof(setupData->sphereId));
	setWithCheck(CS_TYPE::CONFIG_KEY_ADMIN,        &(setupData->adminKey), sizeof(setupData->adminKey));
	setWithCheck(CS_TYPE::CONFIG_KEY_MEMBER,       &(setupData->memberKey), sizeof(setupData->memberKey));
	setWithCheck(CS_TYPE::CONFIG_KEY_BASIC,        &(setupData->basicKey), sizeof(setupData->basicKey));
	setWithCheck(CS_TYPE::CONFIG_KEY_SERVICE_DATA, &(setupData->serviceDataKey), sizeof(setupData->serviceDataKey));
	setWithCheck(CS_TYPE::CONFIG_KEY_LOCALIZATION, &(setupData->localizationKey), sizeof(setupData->localizationKey));
	setWithCheck(CS_TYPE::CONFIG_MESH_DEVICE_KEY,  &(setupData->meshDeviceKey), sizeof(setupData->meshDeviceKey));
	setWithCheck(CS_TYPE::CONFIG_MESH_APP_KEY,     &(setupData->meshAppKey), sizeof(setupData->meshAppKey));
	setWithCheck(CS_TYPE::CONFIG_MESH_NET_KEY,     &(setupData->meshNetKey), sizeof(setupData->meshNetKey));
	setWithCheck(CS_TYPE::CONFIG_IBEACON_UUID,     &(setupData->ibeaconUuid.uuid128), sizeof(setupData->ibeaconUuid.uuid128));
	setWithCheck(CS_TYPE::CONFIG_IBEACON_MAJOR,    &(setupData->ibeaconMajor), sizeof(setupData->ibeaconMajor));
	setWithCheck(CS_TYPE::CONFIG_IBEACON_MINOR,    &(setupData->ibeaconMinor), sizeof(setupData->ibeaconMinor));

	// Make sure the stored switch state is correct, as the switch command might not be executed
	// (for example if the device has no switch, or when the switch is already on).
	// This is necessary because we wait for it to be set.
	TYPIFY(STATE_SWITCH_STATE) switchState;
	switchState.state.dimmer = 0;
	switchState.state.relay = 1;
	setWithCheck(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));

	LOGi("Setup success, wait for storage");
	return ERR_WAIT_FOR_SUCCESS;
}

void Setup::setWithCheck(const CS_TYPE& type, void *value, const size16_t size) {
	cs_ret_code_t retCode = State::getInstance().set(type, value, size);
	LOGSetupDebug("setWithCheck %x=%x (res:%x)", type, *static_cast<uint8_t*>(value), retCode);
	switch (retCode) {
		case ERR_SUCCESS:
			break;
		case ERR_SUCCESS_NO_CHANGE:
			onStorageDone(type);
			break;
		default:
			break;
	}
}

void Setup::onStorageDone(const CS_TYPE& type) {
	LOGd("storage done %u", to_underlying_type(type));
	switch (type) {
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_STONE_ID);
		break;
	case CS_TYPE::CONFIG_SPHERE_ID:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_SPHERE_ID);
		break;
	case CS_TYPE::CONFIG_KEY_ADMIN:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_ADMIN_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_MEMBER:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MEMBER_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_BASIC:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_BASIC_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_SERVICE_DATA:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_SERVICE_DATA_KEY);
		break;
	case CS_TYPE::CONFIG_KEY_LOCALIZATION:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_LOCALIZATION_KEY);
		break;
	case CS_TYPE::CONFIG_MESH_DEVICE_KEY:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MESH_DEVICE_KEY);
		break;
	case CS_TYPE::CONFIG_MESH_APP_KEY:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MESH_APP_KEY);
		break;
	case CS_TYPE::CONFIG_MESH_NET_KEY:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_MESH_NET_KEY);
		break;
	case CS_TYPE::CONFIG_IBEACON_UUID:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_IBEACON_UUID);
		break;
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_IBEACON_MAJOR);
		break;
	case CS_TYPE::CONFIG_IBEACON_MINOR:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_IBEACON_MINOR);
		break;
	case CS_TYPE::STATE_SWITCH_STATE:
		CsUtils::setBit(_successfullyStoredBitmask, SETUP_CONFIG_BIT_SWITCH);
		break;
	case CS_TYPE::STATE_OPERATION_MODE:
		// Check, so that we don't finalize if operation mode was set for some other reason than setup.
		if ((_successfullyStoredBitmask & SETUP_CONFIG_MASK_ALL) == SETUP_CONFIG_MASK_ALL) {
			finalizeSetNormalMode();
		}
		return;
	default:
		break;
	}

	LOGSetupDebug("setupdata successfullyStoredBitmask: %b", _successfullyStoredBitmask);

	if ((_successfullyStoredBitmask & SETUP_CONFIG_MASK_ALL) == SETUP_CONFIG_MASK_ALL) {
		LOGi("All state variables stored");
		setNormalMode();
	}
	else {
		LOGd("Stored: %u all=%u", _successfullyStoredBitmask, SETUP_CONFIG_MASK_ALL);
	}
}

void Setup::setNormalMode() {
	LOGSetupDebug("setNormalMode");

	// Set operation mode to normal mode
	OperationMode operationMode = OperationMode::OPERATION_MODE_NORMAL;
	TYPIFY(STATE_OPERATION_MODE) mode = to_underlying_type(operationMode);

	LOGi("Setting mode to NORMAL: 0x%X", mode);
	setWithCheck(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));

	// Switch relay on
	event_t event(CS_TYPE::CMD_SWITCH_ON);
	event.dispatch();
}

OperationMode Setup::getPersistedOperationMode() {
	LOGSetupDebug("getPersistedOperationMode, reading state from flash");
	TYPIFY(STATE_OPERATION_MODE) mode;

	if (State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode), PersistenceMode::FLASH)
		!= ERR_SUCCESS) {
		LOGw("couldn't read STATE_OPERATION_MODE from flash");
		return OperationMode::OPERATION_MODE_UNINITIALIZED;
	}

	OperationMode operationMode = static_cast<OperationMode>(mode);

	LOGSetupDebug("Operation mode: %s", operationModeName(operationMode));
	return getOperationMode(mode);
}

void Setup::resetDelayed() {
	// reset after delay
	reset_delayed_t resetDelayed;
	resetDelayed.resetCode = CS_RESET_CODE_SOFT_RESET;
	resetDelayed.delayMs = 1000;

	LOGSetupDebug("resetting in %d ms", resetDelayed.delayMs);
	event_t resetEvent(CS_TYPE::CMD_RESET_DELAYED, &resetDelayed, sizeof(resetDelayed));
	resetEvent.dispatch();
}

void Setup::notifyResultAsync(ErrorCodesGeneral errCode) {
	TYPIFY(CMD_RESOLVE_ASYNC_CONTROL_COMMAND) result(CTRL_CMD_SETUP, errCode);
	event_t eventResult(CS_TYPE::CMD_RESOLVE_ASYNC_CONTROL_COMMAND, &result, sizeof(result));
	eventResult.dispatch();
}

void Setup::finalizeSetNormalMode() {
	LOGSetupInfo("Setup::finalize()");

	if (getPersistedOperationMode() == OperationMode::OPERATION_MODE_NORMAL) {
		notifyResultAsync(ERR_SUCCESS);
		resetDelayed();
	} else {
		LOGe("Cannot finalize, operation mode must be NORMAL, retrying %d more times", _retryCount);
		if(_retryCount > 0) {
			_retryCount--;
			setNormalMode();
		// for now continue with reset (will be considered setup mode)
		} else {
			// TODO: notifyResultAsync(ERR_WRONG_MODE);
			resetDelayed();
		}
	}
}

/**
 * This event handler only listens to one event, namely EVT_STORAGE_WRITE_DONE. In that case it will continue with
 * a reset procedure. However, how does it know that the right item has been written? What should be done instead is
 * just calling a reset procedure that - in a loop - queries if there are peripherals still busy. As soon as for
 * example the entire storage queue has been handled, it should perform the reset.
 */
void Setup::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
			TYPIFY(EVT_STORAGE_WRITE_DONE)* eventData = (TYPIFY(EVT_STORAGE_WRITE_DONE)*)event.data;
			onStorageDone(eventData->type);
			break;
		}
		default: {
			break;
		}
	}
}

