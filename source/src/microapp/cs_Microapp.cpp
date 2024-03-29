/** Store "micro" apps in flash.
 *
 * Write a file to flash. This class listens to events and stores them in flash. The location where it is stored in
 * flash is separate from the bluenet binary. It can be seen as a DFU process where bluenet functions as a bootloader
 * for yet other binaries that we call here microapps.
 *
 * A typical microapp can be a small binary compiled with Arduino conventions for the Crownstone architecture.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_UUID.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <drivers/cs_Storage.h>
#include <events/cs_Event.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_Logger.h>
#include <microapp/cs_Microapp.h>
#include <microapp/cs_MicroappController.h>
#include <microapp/cs_MicroappInterruptHandler.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

#include <algorithm>

#define LOGMicroappInfo LOGi
#define LOGMicroappDebug LOGnone
#define LOGMicroappVerbose LOGnone

Microapp::Microapp() : EventListener() {}

void Microapp::init(OperationMode operationMode) {
	switch (operationMode) {
		case OperationMode::OPERATION_MODE_NORMAL: {
			break;
		}
		case OperationMode::OPERATION_MODE_FACTORY_RESET: {
			_factoryResetMode = true;
			break;
		}
		default: {
			return;
		}
	}

	MicroappStorage& storage = MicroappStorage::getInstance();
	storage.init();

	if (!_factoryResetMode) {
		// Set callback handler in IPC ram
		MicroappController& controller = MicroappController::getInstance();
		controller.setIpcRam();

		// Create an instance of the interrupt handler.
		MicroappInterruptHandler::getInstance();

		loadApps();
	}

	EventDispatcher::getInstance().addListener(this);
}

void Microapp::loadApps() {
	// Loop over every app index, as an app might've been uploaded via jlink.
	cs_ret_code_t retCode;
	if (g_AUTO_ENABLE_MICROAPP_ON_BOOT) {
		LOGMicroappInfo("Auto-enabling of apps on boot turned on");
	}
	else {
		LOGMicroappInfo("Auto-enabling of apps on boot turned off");
	}

	for (uint8_t index = 0; index < g_MICROAPP_COUNT; ++index) {
		loadState(index);
		updateStateFromOperatingData(index);
		retCode = validateApp(index);
		if (g_AUTO_ENABLE_MICROAPP_ON_BOOT && retCode == ERR_SUCCESS) {
			LOGMicroappInfo("Enable microapp %u", index);
			enableApp(index);
		}
		if (_states[index].didReboot) {
			LOGMicroappInfo("Sorry, reboot while microapp running. App %u will not be started.", index);
		}
		if (_states[index].exceededCallDuration) {
			LOGMicroappInfo("Sorry, call to microapp took too long to yield. App %u will not be started.", index);
		}
		storeState(index);
		startApp(index);
	}
}

void Microapp::updateStateFromOperatingData(uint8_t index) {
	MicroappController& controller   = MicroappController::getInstance();
	MicroappOperatingState prevState = controller.getOperatingState(index);
	_states[index].didReboot         = (prevState == MicroappOperatingState::CS_MICROAPP_RUNNING);
}

void Microapp::onExcessiveCallDuration(uint8_t appIndex) {
	if (appIndex < g_MICROAPP_COUNT && _states[appIndex].exceededCallDuration == false) {
		LOGw("Disable microapp because the call took too long to yield");
		_states[appIndex].exceededCallDuration = true;
		storeState(appIndex);
	}
}

void Microapp::loadState(uint8_t index) {
	LOGMicroappDebug("loadState %u", index);
	cs_ret_code_t retCode;
	cs_state_data_t stateData(
			CS_TYPE::STATE_MICROAPP, index, reinterpret_cast<uint8_t*>(&(_states[index])), sizeof(_states[0]));
	retCode = State::getInstance().get(stateData);
	if (retCode != ERR_SUCCESS) {
		resetState(index);
	}

	// TODO: we only need this because you may have flashed the microapp on the chip.
	MicroappStorage& storage = MicroappStorage::getInstance();
	_states[index].hasData   = !storage.isErased(index);
}

/*
 * Validate the app.
 *
 * First return a possible error of an incorrect checksum. Only then return when there is no proper state information
 * available. This means that it can be used when AUTO_ENABLE_MICROAPP_ON_BOOT is set.
 */
cs_ret_code_t Microapp::validateApp(uint8_t index) {
	LOGMicroappDebug("validateApp %u", index);
	cs_ret_code_t retCode;
	microapp_state_t& state  = _states[index];

	MicroappStorage& storage = MicroappStorage::getInstance();
	retCode                  = storage.validateApp(index);
	if (retCode != ERR_SUCCESS) {
		LOGMicroappInfo("No valid app binary on index %u", index);
		state.checksumTest = MICROAPP_TEST_STATE_FAILED;
		return retCode;
	}

	if (!state.hasData) {
		LOGMicroappInfo("Microapp %u has no data", index);
		return ERR_WRONG_STATE;
	}

	microapp_binary_header_t appHeader;
	storage.getAppHeader(index, appHeader);
	if (state.checksum != appHeader.checksum || state.checksumHeader != appHeader.checksumHeader) {
		LOGMicroappInfo(
				"New app on index %u: stored checksum %u and %u, binary checksum %u and %u",
				index,
				state.checksum,
				state.checksumHeader,
				appHeader.checksum,
				appHeader.checksumHeader);
		resetState(index);
	}
	else {
		LOGMicroappInfo("Known app on index %u", index);
	}
	state.checksum       = appHeader.checksum;
	state.checksumHeader = appHeader.checksumHeader;
	state.checksumTest   = MICROAPP_TEST_STATE_PASSED;
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::enableApp(uint8_t index) {
	microapp_state_t& state = _states[index];

	if (state.checksumTest != MICROAPP_TEST_STATE_PASSED) {
		LOGMicroappInfo("app %u checksum did not pass yet", index);
		return ERR_WRONG_STATE;
	}

	microapp_binary_header_t header;
	MicroappStorage& storage = MicroappStorage::getInstance();
	storage.getAppHeader(index, header);
	if (header.sdkVersionMajor != MICROAPP_SDK_MAJOR || header.sdkVersionMinor > MICROAPP_SDK_MINOR) {
		LOGw("Microapp sdk version %u.%u is not supported (it is >%u.%u)",
			 header.sdkVersionMajor,
			 header.sdkVersionMinor,
			 MICROAPP_SDK_MAJOR,
			 MICROAPP_SDK_MINOR);
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	state.enabled = true;
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::startApp(uint8_t index) {
	if (!canRunApp(index)) {
		LOGMicroappInfo(
				"Can't run app: enabled=%u checkSumTest=%u memoryUsage=%u bootTest=%u didReboot=%u exceededCallDuration=%u failedFunction=%u",
				_states[index].enabled,
				_states[index].checksumTest,
				_states[index].memoryUsage,
				_states[index].bootTest,
				_states[index].didReboot,
				_states[index].exceededCallDuration,
				_states[index].failedFunction);
		return ERR_UNSAFE;
	}
	if (_started[index]) {
		return ERR_SUCCESS;
	}

	LOGMicroappInfo("startApp %u", index);
	MicroappController::getInstance().startMicroapp(index);
	_started[index] = true;
	return ERR_SUCCESS;
}

void Microapp::resetState(uint8_t index) {
	memset(&(_states[index]), 0, sizeof(_states[0]));
	_states[index].tryingFunction = MICROAPP_FUNCTION_NONE;
	_states[index].failedFunction = MICROAPP_FUNCTION_NONE;
}

void Microapp::resetTestState(uint8_t index) {
	_states[index].checksumTest         = MICROAPP_TEST_STATE_UNTESTED;
	_states[index].memoryUsage          = MICROAPP_TEST_STATE_UNTESTED;
	_states[index].tryingFunction       = MICROAPP_FUNCTION_NONE;
	_states[index].failedFunction       = MICROAPP_FUNCTION_NONE;
	_states[index].passedFunctions      = 0;
	_states[index].didReboot            = false;
	_states[index].exceededCallDuration = false;

	// Not resetting _started[index], otherwise the app gets started again on enable.
	// While the registered soft interrtups etc were not cleaned up on disable.
}

cs_ret_code_t Microapp::storeState(uint8_t index) {
	LOGMicroappVerbose("storeState %u", index);
	cs_state_data_t stateData(
			CS_TYPE::STATE_MICROAPP, index, reinterpret_cast<uint8_t*>(&(_states[index])), sizeof(_states[0]));
	cs_ret_code_t retCode = State::getInstance().set(stateData);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE: {
			return ERR_SUCCESS;
		}
		default: return retCode;
	}
}

/*
 * An app is allowed to run when the following conditions are met:
 *
 *   - The app should be enabled.
 *   - The checksum should be correct.
 *   - Memory usage should be unequal to 1? TODO: Explain.
 *   - There should be a failed boot test? TODO: Explain.
 *   - There shouldn't be a failed function? TODO: Explain.
 */
bool Microapp::canRunApp(uint8_t index) {
	LOGMicroappVerbose(
			"checkSumTest=%u enabled=%u bootTest=%u memoryUsage=%u didReboot=%u exceededCallDuration=%u failedFunction=%u",
			_states[index].checksumTest,
			_states[index].enabled,
			_states[index].bootTest,
			_states[index].memoryUsage,
			_states[index].didReboot,
			_states[index].exceededCallDuration,
			_states[index].failedFunction);
	return _states[index].enabled && _states[index].checksumTest == MICROAPP_TEST_STATE_PASSED
		   && _states[index].memoryUsage != 1 && _states[index].bootTest != MICROAPP_TEST_STATE_FAILED
		   && _states[index].didReboot != true && _states[index].exceededCallDuration != true
		   && _states[index].failedFunction == MICROAPP_FUNCTION_NONE;
}

void Microapp::tick() {
	MicroappController& controller = MicroappController::getInstance();
	for (uint8_t i = 0; i < g_MICROAPP_COUNT; ++i) {
		if (canRunApp(i)) {
			controller.tickMicroapp(i);
		}
	}
	if (_factoryResetMode && _currentMicroappIndex != MICROAPP_INDEX_NONE) {
		// We can just try to resume all the time, as it will just return BUSY otherwise.
		resumeFactoryReset();
	}
}

cs_ret_code_t Microapp::handleGetInfo(cs_result_t& result) {
	LOGMicroappInfo("handleGetInfo");
	microapp_info_t* info = reinterpret_cast<microapp_info_t*>(result.buf.data);
	if (result.buf.len < sizeof(*info)) {
		LOGw("Buffer too small len=%u required=%u", result.buf.len, sizeof(*info));
		return ERR_BUFFER_TOO_SMALL;
	}

	info->protocol           = MICROAPP_CONTROL_COMMAND_PROTOCOL;
	info->maxApps            = g_MICROAPP_COUNT;
	info->maxAppSize         = MICROAPP_MAX_SIZE;
	info->maxChunkSize       = MICROAPP_UPLOAD_MAX_CHUNK_SIZE;
	info->maxRamUsage        = g_RAM_MICROAPP_AMOUNT;
	info->sdkVersion.major   = MICROAPP_SDK_MAJOR;
	info->sdkVersion.minor   = MICROAPP_SDK_MINOR;

	MicroappStorage& storage = MicroappStorage::getInstance();
	microapp_binary_header_t appHeader;
	for (uint8_t index = 0; index < g_MICROAPP_COUNT; ++index) {
		storage.getAppHeader(index, appHeader);
		info->appsStatus[index].buildVersion     = appHeader.appBuildVersion;
		info->appsStatus[index].sdkVersion.major = appHeader.sdkVersionMajor;
		info->appsStatus[index].sdkVersion.minor = appHeader.sdkVersionMinor;
		memcpy(&(info->appsStatus[index].state), &(_states[index]), sizeof(_states[0]));
	}
	result.dataSize = sizeof(*info);
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::handleUpload(microapp_upload_internal_t* packet) {
	LOGMicroappInfo("handleUpload index=%u offset=%u", packet->header.header.index, packet->header.offset);
	cs_ret_code_t retCode = checkHeader(&(packet->header.header));
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	uint8_t index = packet->header.header.index;

	// The previous app at this index will not run anymore, so reset the state.
	resetState(index);

	MicroappStorage& storage = MicroappStorage::getInstance();
	// CAREFUL: This assumes the data stays in ram during the write.
	retCode =
			storage.writeChunk(packet->header.header.index, packet->header.offset, packet->data.data, packet->data.len);

	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE:
		case ERR_WAIT_FOR_SUCCESS: {
			// In case of wait: assume the flash will be written.
			_states[index].hasData = true;
			break;
		}
		case ERR_WRITE_DISABLED: {
			// Specific error we get when the storage space at this chunk is not empty, update state.
			_states[index].hasData = true;
			break;
		}
	}

	storeState(index);
	return retCode;
}

cs_ret_code_t Microapp::handleValidate(microapp_ctrl_header_t* packet) {
	LOGMicroappInfo("handleValidate %u", packet->index);
	cs_ret_code_t retCode = checkHeader(packet);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	uint8_t index = packet->index;
	retCode       = validateApp(index);
	storeState(index);
	if (retCode == ERR_SUCCESS) {
		startApp(index);
	}
	return retCode;
}

cs_ret_code_t Microapp::handleRemove(microapp_ctrl_header_t* packet) {
	LOGMicroappInfo("handleRemove index=%u", packet->index);
	cs_ret_code_t retCode = checkHeader(packet);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	uint8_t index          = packet->index;

	// First, stop running the microapp.
	_states[index].enabled = false;

	// Clean up the microapp.
	MicroappController::getInstance().clear(index);

	MicroappStorage& storage = MicroappStorage::getInstance();
	retCode                  = storage.erase(index);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_WAIT_FOR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE: break;
		default: return retCode;
	}

	// Assume the erase will succeed. If not, the state will be corrected later.
	//	_states[index].hasData = false;
	_started[index] = false;
	resetState(index);
	storeState(index);
	return retCode;
}

cs_ret_code_t Microapp::handleEnable(microapp_ctrl_header_t* packet) {
	LOGMicroappInfo("handleEnable %u", packet->index);
	cs_ret_code_t retCode = checkHeader(packet);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	uint8_t index = packet->index;

	retCode       = enableApp(index);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	// The enable command resets all test, except occupied field and checksum test.
	resetTestState(index);
	_states[index].hasData      = true;
	_states[index].checksumTest = MICROAPP_TEST_STATE_PASSED;

	retCode                     = storeState(index);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	return startApp(index);
}

cs_ret_code_t Microapp::handleDisable(microapp_ctrl_header_t* packet) {
	LOGMicroappInfo("handleDisable %u", packet->index);
	cs_ret_code_t retCode = checkHeader(packet);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	uint8_t index          = packet->index;
	_states[index].enabled = false;
	return storeState(index);
}

cs_ret_code_t Microapp::handleMessage(microapp_message_internal_t* packet, cs_result_t& result) {
	LOGMicroappInfo("handleMessage appIndex=%u", packet->header.index);
	cs_ret_code_t retCode = checkHeader(&packet->header);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	return MicroappInterruptHandler::getInstance().onMessage(
			packet->header.index,
			packet->payload,
			result.buf,
			result.dataSize);
}

cs_ret_code_t Microapp::checkHeader(microapp_ctrl_header_t* packet) {
	LOGMicroappDebug("checkHeader %u", packet->index);
	if (packet->protocol != MICROAPP_CONTROL_COMMAND_PROTOCOL) {
		LOGw("Unsupported protocol: %u", packet->protocol);
		return ERR_PROTOCOL_UNSUPPORTED;
	}
	if (packet->index > g_MICROAPP_COUNT) {
		LOGw("Index too large: %u", packet->index);
		return ERR_WRONG_PARAMETER;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::factoryReset() {
	if (!_factoryResetMode) {
		// Should not happen.
		// Maybe send event done with this as result code instead.
		return ERR_WRONG_MODE;
	}
	_currentMicroappIndex = 0;
	return resumeFactoryReset();
}

cs_ret_code_t Microapp::resumeFactoryReset() {
	LOGd("resumeFactoryReset index=%u", _currentMicroappIndex);
	if (_currentMicroappIndex == MICROAPP_INDEX_NONE) {
		return ERR_WRONG_STATE;
	}
	if (_currentMicroappIndex == g_MICROAPP_COUNT) {
		// Done
		_currentMicroappIndex = MICROAPP_INDEX_NONE;
		event_t event(CS_TYPE::EVT_MICROAPP_FACTORY_RESET_DONE);
		event.dispatch();
		return ERR_SUCCESS;
	}
	cs_ret_code_t retCode = MicroappStorage::getInstance().erase(_currentMicroappIndex);
	switch (retCode) {
		case ERR_SUCCESS: {
			// Page was already erased, continue immediately.
			_currentMicroappIndex++;
			return resumeFactoryReset();
		}
		case ERR_BUSY: {
			// Retry again later, done in tick();
			break;
		}
		case ERR_WAIT_FOR_SUCCESS: {
			// Wait for storage event.
			break;
		}
		default: {
			// Now what? Just assume success for now, because we don't want to get stuck.
			LOGe("Erase failed, but assuming success retCode=%u", retCode);
			_currentMicroappIndex++;
			return resumeFactoryReset();
		}
	}
	return ERR_WAIT_FOR_SUCCESS;
}

void Microapp::onStorageEvent(cs_async_result_t& event) {
	if (event.commandType != CTRL_CMD_MICROAPP_REMOVE) {
		return;
	}

	if (!_factoryResetMode) {
		return;
	}

	if (_currentMicroappIndex == MICROAPP_INDEX_NONE) {
		return;
	}

	_currentMicroappIndex++;
	resumeFactoryReset();
}

void Microapp::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::CMD_MICROAPP_GET_INFO: {
			evt.result.returnCode = handleGetInfo(evt.result);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_UPLOAD: {
			auto packet           = CS_TYPE_CAST(CMD_MICROAPP_UPLOAD, evt.data);
			evt.result.returnCode = handleUpload(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_VALIDATE: {
			auto packet           = CS_TYPE_CAST(CMD_MICROAPP_VALIDATE, evt.data);
			evt.result.returnCode = handleValidate(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_REMOVE: {
			auto packet           = CS_TYPE_CAST(CMD_MICROAPP_REMOVE, evt.data);
			evt.result.returnCode = handleRemove(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_ENABLE: {
			auto packet           = CS_TYPE_CAST(CMD_MICROAPP_ENABLE, evt.data);
			evt.result.returnCode = handleEnable(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_DISABLE: {
			auto packet           = CS_TYPE_CAST(CMD_MICROAPP_DISABLE, evt.data);
			evt.result.returnCode = handleDisable(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_MESSAGE: {
			auto data             = CS_TYPE_CAST(CMD_MICROAPP_MESSAGE, evt.data);
			evt.result.returnCode = handleMessage(data, evt.result);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			tick();
			break;
		}
		case CS_TYPE::CMD_FACTORY_RESET: {
			evt.result.returnCode = factoryReset();
			break;
		}
		case CS_TYPE::CMD_RESOLVE_ASYNC_CONTROL_COMMAND: {
			auto packet = CS_TYPE_CAST(CMD_RESOLVE_ASYNC_CONTROL_COMMAND, evt.data);
			onStorageEvent(*packet);
			break;
		}
		default: {
			// ignore other events
		}
	}
}
