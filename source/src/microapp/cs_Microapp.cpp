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


#include <algorithm>
#include <ble/cs_UUID.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <microapp/cs_Microapp.h>
#include <microapp/cs_MicroappProtocol.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

#define LOGMicroappInfo LOGi
#define LOGMicroappDebug LOGnone
#define LOGMicroappVerbose LOGnone

Microapp::Microapp(): EventListener() {
}

void Microapp::init() {
	MicroappStorage & storage = MicroappStorage::getInstance();
	storage.init();

	// Set callback handler in IPC ram
	MicroappProtocol & protocol = MicroappProtocol::getInstance();
	protocol.setIpcRam();

	loadApps();

	EventDispatcher::getInstance().addListener(this);
}

void Microapp::loadApps() {
	// Loop over every app index, as an app might've been uploaded via jlink.
	cs_ret_code_t retCode;
	for (uint8_t index = 0; index < MAX_MICROAPPS; ++index) {
		loadState(index);
		retCode = validateApp(index);
		if (g_AUTO_ENABLE_MICROAPP_ON_BOOT && retCode == ERR_SUCCESS) {
			LOGMicroappInfo("Enable microapp %u", index);
			enableApp(index);
		}
		storeState(index);
		startApp(index);
	}
}

void Microapp::loadState(uint8_t index) {
	LOGMicroappDebug("loadState %u", index);
	cs_ret_code_t retCode;
	cs_state_data_t stateData(CS_TYPE::STATE_MICROAPP, index, reinterpret_cast<uint8_t*>(&(_states[index])), sizeof(_states[0]));
	retCode = State::getInstance().get(stateData);
	if (retCode != ERR_SUCCESS) {
		resetState(index);
	}

	// TODO: we only need this because you may have flashed the microapp on the chip.
	MicroappStorage & storage = MicroappStorage::getInstance();
	_states[index].hasData = !storage.isErased(index);
}

cs_ret_code_t Microapp::validateApp(uint8_t index) {
	LOGMicroappDebug("validateApp %u", index);
	cs_ret_code_t retCode;
	microapp_state_t & state = _states[index];

	if (!state.hasData) {
		LOGMicroappInfo("app %u is erased", index);
		return ERR_WRONG_STATE;
	}

	MicroappStorage & storage = MicroappStorage::getInstance();
	retCode = storage.validateApp(index);
	if (retCode != ERR_SUCCESS) {
		LOGMicroappInfo("No valid app binary on index %u", index);
		state.checksumTest = MICROAPP_TEST_STATE_FAILED;
		return retCode;
	}

	microapp_binary_header_t appHeader;
	storage.getAppHeader(index, appHeader);
	if (state.checksum != appHeader.checksum || state.checksumHeader != appHeader.checksumHeader) {
		LOGMicroappInfo("New app on index %u: stored checksum %u and %u, binary checksum %u and %u",
				index,
				state.checksum,
				state.checksumHeader,
				appHeader.checksum,
				appHeader.checksumHeader);
		resetState(index);
	}
	state.checksum = appHeader.checksum;
	state.checksumHeader = appHeader.checksumHeader;
	state.checksumTest = MICROAPP_TEST_STATE_PASSED;
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::enableApp(uint8_t index) {
	microapp_state_t & state = _states[index];

	if (state.checksumTest != MICROAPP_TEST_STATE_PASSED) {
		LOGMicroappInfo("app %u checksum did not pass yet", index);
		return ERR_WRONG_STATE;
	}

	microapp_binary_header_t header;
	MicroappStorage & storage = MicroappStorage::getInstance();
	storage.getAppHeader(index, header);
	if (header.sdkVersionMajor != MICROAPP_SDK_MAJOR || header.sdkVersionMinor > MICROAPP_SDK_MINOR) {
		LOGw("Microapp sdk version %u.%u is not supported", header.sdkVersionMajor, header.sdkVersionMinor);
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	state.enabled = true;
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::startApp(uint8_t index) {
	LOGMicroappInfo("startApp %u", index);
	if (!canRunApp(index)) {
		LOGMicroappDebug("Can't run app: enabled=%u checkSumTest=%u memoryUsage=%u bootTest=%u failedFunction=%u",
				_states[index].enabled,
				_states[index].checksumTest,
				_states[index].memoryUsage,
				_states[index].bootTest,
				_states[index].failedFunction);
		return ERR_UNSAFE;
	}
	MicroappProtocol & protocol = MicroappProtocol::getInstance();
	protocol.callApp(index);
	return ERR_SUCCESS;
}

void Microapp::resetState(uint8_t index) {
	memset(&(_states[index]), 0, sizeof(_states[0]));
	_states[index].tryingFunction = MICROAPP_FUNCTION_NONE;
	_states[index].failedFunction = MICROAPP_FUNCTION_NONE;
}

void Microapp::resetTestState(uint8_t index) {
	_states[index].checksumTest = MICROAPP_TEST_STATE_UNTESTED;
	_states[index].memoryUsage = MICROAPP_TEST_STATE_UNTESTED;
	_states[index].tryingFunction = MICROAPP_FUNCTION_NONE;
	_states[index].failedFunction = MICROAPP_FUNCTION_NONE;
	_states[index].passedFunctions = 0;
}

cs_ret_code_t Microapp::storeState(uint8_t index) {
	LOGMicroappVerbose("storeState %u", index);
	cs_state_data_t stateData(CS_TYPE::STATE_MICROAPP, index, reinterpret_cast<uint8_t*>(&(_states[index])), sizeof(_states[0]));
	cs_ret_code_t retCode = State::getInstance().set(stateData);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE: {
			return ERR_SUCCESS;
		}
		default:
			return retCode;
	}
}

bool Microapp::canRunApp(uint8_t index) {
	LOGMicroappVerbose("enabled=%u checkSumTest=%u memoryUsage=%u bootTest=%u failedFunction=%u",
			_states[index].enabled,
			_states[index].checksumTest,
			_states[index].memoryUsage,
			_states[index].bootTest,
			_states[index].failedFunction);
	return _states[index].enabled &&
			_states[index].checksumTest == MICROAPP_TEST_STATE_PASSED &&
			_states[index].memoryUsage != 1 &&
			_states[index].bootTest != MICROAPP_TEST_STATE_FAILED &&
			_states[index].failedFunction == MICROAPP_FUNCTION_NONE;
}

void Microapp::tick() {
	MicroappProtocol & protocol = MicroappProtocol::getInstance();
	for (uint8_t i = 0; i < MAX_MICROAPPS; ++i) {
		if (canRunApp(i)) {
			protocol.callSetupAndLoop(i);
		}
	}
}

cs_ret_code_t Microapp::handleGetInfo(cs_result_t& result) {
	LOGMicroappInfo("handleGetInfo");
	microapp_info_t* info = reinterpret_cast<microapp_info_t*>(result.buf.data);
	if (result.buf.len < sizeof(*info)) {
		LOGw("Buffer too small len=%u required=%u", result.buf.len, sizeof(*info));
		return ERR_BUFFER_TOO_SMALL;
	}

	info->protocol = MICROAPP_PROTOCOL;
	info->maxApps = MAX_MICROAPPS;
	info->maxAppSize = MICROAPP_MAX_SIZE;
	info->maxChunkSize = MICROAPP_UPLOAD_MAX_CHUNK_SIZE;
	info->maxRamUsage = MICROAPP_MAX_RAM;
	info->sdkVersion.major = MICROAPP_SDK_MAJOR;
	info->sdkVersion.minor = MICROAPP_SDK_MINOR;

	MicroappStorage & storage = MicroappStorage::getInstance();
	microapp_binary_header_t appHeader;
	for (uint8_t index = 0; index < MAX_MICROAPPS; ++index) {
		storage.getAppHeader(index, appHeader);
		info->appsStatus[index].buildVersion = appHeader.appBuildVersion;
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

	MicroappStorage & storage = MicroappStorage::getInstance();
	// CAREFUL: This assumes the data stays in ram during the write.
	retCode = storage.writeChunk(packet->header.header.index, packet->header.offset, packet->data.data, packet->data.len);

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
	retCode = validateApp(index);
	storeState(index);
	if (retCode == ERR_SUCCESS) {
		startApp(index);
	}
	return retCode;
}

cs_ret_code_t Microapp::handleRemove(microapp_ctrl_header_t* packet) {
	LOGMicroappInfo("handleRemove %u", packet->index);
	cs_ret_code_t retCode = checkHeader(packet);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	uint8_t index = packet->index;

	MicroappStorage & storage = MicroappStorage::getInstance();
	retCode = storage.erase(index);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_WAIT_FOR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE:
			break;
		default:
			return retCode;
	}

	// Assume the erase will succeed. If not, the state will be corrected later.
//	_states[index].hasData = false;
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

	retCode = enableApp(index);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	// The enable command resets all test, except occupied field and checksum test.
	resetTestState(index);
	_states[index].hasData = true;
	_states[index].checksumTest = MICROAPP_TEST_STATE_PASSED;

	retCode = storeState(index);
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

	uint8_t index = packet->index;
	_states[index].enabled = false;
	return storeState(index);
}

cs_ret_code_t Microapp::checkHeader(microapp_ctrl_header_t* packet) {
	if (packet->protocol != MICROAPP_PROTOCOL) {
		LOGw("Unsupported protocol: %u", packet->protocol);
		return ERR_PROTOCOL_UNSUPPORTED;
	}
	if (packet->index > MAX_MICROAPPS) {
		LOGw("Index too large: %u", packet->index);
		return ERR_WRONG_PARAMETER;
	}
	return ERR_SUCCESS;
}

void Microapp::handleEvent(event_t & evt) {
	switch (evt.type) {
		case CS_TYPE::CMD_MICROAPP_GET_INFO: {
			evt.result.returnCode = handleGetInfo(evt.result);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_UPLOAD: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_UPLOAD, evt.data);
			evt.result.returnCode = handleUpload(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_VALIDATE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_VALIDATE, evt.data);
			evt.result.returnCode = handleValidate(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_REMOVE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_REMOVE, evt.data);
			evt.result.returnCode = handleRemove(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_ENABLE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_ENABLE, evt.data);
			evt.result.returnCode = handleEnable(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_DISABLE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_DISABLE, evt.data);
			evt.result.returnCode = handleDisable(packet);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			tick();
			break;
		}
		default: {
			// ignore other events
		}
	}
}
