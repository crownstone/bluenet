/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <dfu/cs_FirmwareSections.h>
#include <dfu/cs_MeshDfuConstants.h>
#include <dfu/cs_MeshDfuHost.h>
#include <logging/cs_Logger.h>

#define LOGMeshDfuHostDebug LOGd
#define LOGMeshDfuHostInfo LOGi
#define LOGMeshDfuHostWarn LOGw

// -------------------------------------------------------------------------------------
// ---------------------------------- public methods -----------------------------------
// -------------------------------------------------------------------------------------

#define ________PUBLIC_METHODS________

cs_ret_code_t MeshDfuHost::init() {
	LOGMeshDfuHostDebug("init");

	_bleCentral        = &BleCentral::getInstance();
	_crownstoneCentral = getComponent<CrownstoneCentral>();
	_firmwareReader    = getComponent<FirmwareReader>();

	if (!_listening) {
		LOGMeshDfuHostDebug("Start listening");
		listen();
		_listening = true;
	}

	if (!isInitialized()) {
		LOGMeshDfuHostDebug(
				"MeshDfuHost failed to initialize, blecentral: %0x, cronwstonecentral: %0x",
				_bleCentral,
				_crownstoneCentral);
		return ERR_NOT_INITIALIZED;
	}

	loadInitPacketLen();

	if (!haveInitPacket()) {
		LOGMeshDfuHostDebug("MeshDfuHost no init packet available");
		return ERR_NOT_AVAILABLE;
	}

	auto result = _meshDfuTransport.init();
	if (result != ERR_SUCCESS) {
		LOGMeshDfuHostDebug("mesh dfu transport failed to initialize");
		return result;
	}

	_timeOutRoutine._action = [this]() {
		onEventCallbackTimeOut();
		// (timeoutroutine is intended as single shot but have to return a delay value)
		return Coroutine::delayS(10);
	};
	_timeOutRoutine.pause();

	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;

	LOGMeshDfuHostDebug("MeshDfuHost init successful");

	startPhase(Phase::Idle);
	return ERR_SUCCESS;
}

void MeshDfuHost::handleEvent(event_t& event) {

	// connection events always need to update their local _isConnected prior to any event callback handling.
	switch (event.type) {
		case CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT: {
			cs_ret_code_t* connectResult  = CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
			_isCrownstoneCentralConnected = *connectResult == ERR_SUCCESS;

			LOGMeshDfuHostDebug(
					"Crownstone central connect result received: result %d. Isconnected: %d",
					*connectResult,
					_isCrownstoneCentralConnected);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			[[maybe_unused]] cs_ret_code_t* connectResult = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);

			if (_bleCentral != nullptr) {
				LOGMeshDfuHostDebug(
						"BLE central connect result received: result %d. Isconnected: %d",
						*connectResult,
						_bleCentral->isConnected());
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED: {
			LOGMeshDfuHostDebug("Ble central disconnected, setting _isCrownstoneCentralConnected to false.");
			_isCrownstoneCentralConnected = false;
			break;
		}
		default: {
			break;
		}
	}

	// if a callback is waiting on this event call it back.
	if (_onExpectedEvent != nullptr && event.type == _expectedEvent) {
		// create local copy
		auto eventCallback = _onExpectedEvent;

		// clear expected event
		_onExpectedEvent = nullptr;
		(this->*eventCallback)(event);
	}

	_timeOutRoutine.handleEvent(event);

	// DEBUG
	// this is solely to initiate a copyFirmwareTo call after boot time-out.
	if (event.type == CS_TYPE::EVT_TICK) {
		if (CsMath::Decrease(ticks_until_start) == 1) {
			LOGMeshDfuHostDebug("starting dfu process");
			copyFirmwareTo(_debugTarget);
		}
		if (ticks_until_start % 10 == 1) {
			LOGMeshDfuHostDebug("tick counting: %u ", ticks_until_start);
		}
	}
	// DEBUG END
}

bool MeshDfuHost::copyFirmwareTo(device_address_t target) {
	LOGMeshDfuHostDebug("+++ Copy firmware to target");
	if (!isInitialized()) {
		return init() == ERR_SUCCESS;
	}

	if (!ableToLaunchDfu()) {
		LOGMeshDfuHostWarn("+++ no init packet available");
		return false;
	}

	if(_phaseCurrent != Phase::Idle) {
		LOGMeshDfuHostWarn("+++ copyFirmwareTo called but not  MeshDfuHost is not idle");
		return false;
	}

	_targetDevice = target;

	return startPhase(Phase::ConnectTargetInDfuMode);
	//	return startPhase(Phase::TargetTriggerDfuMode);
}

// -------------------------------------------------------------------------------------
// ------------------------------- stream implementation -------------------------------
// -------------------------------------------------------------------------------------
#define ________STREAM_IMPLEMENTATION________

void MeshDfuHost::stream() {
	LOGMeshDfuHostDebug("steaming: startOffset 0x%08X, bytes left: %u", _streamNextWriteOffset, _streamLeftToWrite);

	if (_streamLeftToWrite == 0 || _streamSection == FirmwareSection::Unknown) {
		LOGMeshDfuHostDebug("done streaming, bytes left is 0 or section unknown.");
		clearStreamState();
		completePhase();
		return;
	}

	cs_data_t buff = _bleCentral->requestWriteBuffer();

	if (buff.data == nullptr || buff.len == 0) {
		if (CsMath::Decrease(_reconnectionAttemptsLeft) > 0) {
			LOGMeshDfuHostInfo("Dfu stream couldn't aqcuire buffer, retrying %u more times", _reconnectionAttemptsLeft);
			setTimeoutCallback(&MeshDfuHost::stream, MeshDfuConstants::DfuHostSettings::ReconnectTimeoutMs);
		}
		else {
			LOGMeshDfuHostWarn("Dfu stream couldn't aqcuire buffer, aborting.");
			abort();
		}

		return;
	}

	_streamCurrentChunkSize =
			CsMath::min(CsMath::min(_streamLeftToWrite, buff.len), MeshDfuConstants::DFUAdapter::LOCAL_ATT_MTU);

	buff.len                 = _streamCurrentChunkSize;
	cs_ret_code_t readResult = _firmwareReader->read(_streamNextWriteOffset, buff.len, buff.data, _streamSection);

	if (readResult != ERR_SUCCESS) {
		if (CsMath::Decrease(_reconnectionAttemptsLeft) > 0) {
			LOGMeshDfuHostInfo(
					"Dfu stream couldn't read required data into buffer, retrying %u more times",
					_reconnectionAttemptsLeft);
			setTimeoutCallback(&MeshDfuHost::stream, MeshDfuConstants::DfuHostSettings::ReconnectTimeoutMs);
		}
		else {
			LOGMeshDfuHostWarn("Dfu stream couldn't read required data into buffer, aborting.");
			abort();
		}

		return;
	}

	LOGMeshDfuHostDebug("writing data chunk of size %u", _streamCurrentChunkSize);
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT, &MeshDfuHost::onStreamResult);
	setTimeoutCallback(&MeshDfuHost::abort);
	_meshDfuTransport.write_data_point(buff);
}

void MeshDfuHost::onStreamResult(event_t& event) {
	TYPIFY(EVT_BLE_CENTRAL_WRITE_RESULT) result = *CS_TYPE_CAST(EVT_BLE_CENTRAL_WRITE_RESULT, event.data);
	LOGMeshDfuHostDebug("stream result received: %u", static_cast<uint8_t>(result));

	// check result
	// on ok, update write data

	if (result == ERR_SUCCESS) {
		// update state
		_streamNextWriteOffset += _streamCurrentChunkSize;
		_streamLeftToWrite -= _streamCurrentChunkSize;
		_streamCurrentChunkSize = 0;

		// stream more
		setTimeoutCallback(&MeshDfuHost::stream, MeshDfuConstants::DfuHostSettings::StreamIntervalMs);
		stream();
	}
	else {
		// @Bart: what errors can be expected here and when can I be sure no data has been sent to the
		// target device so that I can try again?

		abort();
		return;
	}
}

void MeshDfuHost::setStreamState(
		FirmwareSection streamSection, uint32_t streamNextWriteOffset, uint32_t streamLeftToWrite, uint32_t streamCrc) {
	_streamSection         = streamSection;
	_streamNextWriteOffset = streamNextWriteOffset;
	_streamLeftToWrite     = streamLeftToWrite;
	_streamCrc             = streamCrc;
}

void MeshDfuHost::clearStreamState() {
	setStreamState(FirmwareSection::Unknown, 0, 0, 0);
	_streamCurrentChunkSize = 0;
}

// -------------------------------------------------------------------------------------
// ------------------------------- phase implementations -------------------------------
// -------------------------------------------------------------------------------------

#define ________PHASE_IMPLEMENTATIONS________

// ###### TargetTriggerDfuMode ######

bool MeshDfuHost::startPhaseIdle() {
	reset();
	return true;
}

// ###### TargetTriggerDfuMode ######
#define _PHASE_TargetTriggerDfuMode_

bool MeshDfuHost::startPhaseTargetTriggerDfuMode() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetTriggerDfuMode");

	if (_reconnectionAttemptsLeft > 0) {
		CsMath::Decrease(_reconnectionAttemptsLeft);

		setEventCallback(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::sendDfuCommand);
		setTimeoutCallback(&MeshDfuHost::restartPhase);

		auto status = _crownstoneCentral->connect(_targetDevice);

		switch (status) {
			case ERR_BUSY: {
				LOGMeshDfuHostDebug(
						"+++ CrownstoneCentral busy. Retrying after timeout. %u attempts left",
						_reconnectionAttemptsLeft);
				break;
			}
			case ERR_WRONG_STATE: {
				LOGw("+++ CrownstoneCentral is in wrong state. Retrying after timeout. %u attempts left",
					 _reconnectionAttemptsLeft);
				break;
			}
			case ERR_WAIT_FOR_SUCCESS: {
				LOGMeshDfuHostDebug("+++ waiting for crownstone central connect result");
				break;
			}
			default: {
				LOGMeshDfuHostDebug("Unknown returnvalue");
			}
		}

		return true;
	}
	else {
		LOGMeshDfuHostDebug("+++ Can't connect to dfu target. Aborting.");
		return false;
	}
}

void MeshDfuHost::sendDfuCommand(event_t& event) {
	if (!_isCrownstoneCentralConnected) {
		LOGMeshDfuHostDebug("+++ crownstone central not connected.");
		// this will abort if _reconnectionAttemptsLeft reaches zero.
		setTimeoutCallback(&MeshDfuHost::restartPhase, 10000);
		return;
	}

	LOGMeshDfuHostDebug("+++ sendDfuCommand");

	// expect a disconnection after writing the control command
	// this needs to be the BLE_CENTRAL event because CrownstoneCentral may not have an
	// active operation.
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::verifyDisconnectAfterDfu);
	setTimeoutCallback(&MeshDfuHost::abort);

	cs_ret_code_t result = _crownstoneCentral->write(CommandHandlerTypes::CTRL_CMD_GOTO_DFU, nullptr, 0);

	if (result != ERR_WAIT_FOR_SUCCESS) {
		// this will abort if _reconnectionAttemptsLeft reaches zero.
		setTimeoutCallback(&MeshDfuHost::restartPhase, 10000);
		return;
	}

	LOGMeshDfuHostDebug("+++ waiting for disconnect after command goto dfu");
}

void MeshDfuHost::verifyDisconnectAfterDfu(event_t& event) {
	if (_isCrownstoneCentralConnected) {
		LOGMeshDfuHostDebug("+++ incorrect state, expected to be disconnected after dfu command.");
		setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::verifyDisconnectAfterDfu);
		LOGMeshDfuHostDebug("+++ waiting for disconnect after command goto dfu");
		return;
	}

	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetTriggerDfuMode() {
	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
	return Phase::WaitForTargetReboot;
}

// ###### WaitForTargetReboot ######
#define _PHASE_WaitForTargetReboot_

bool MeshDfuHost::startWaitForTargetReboot() {
	// start this phase waiting for a scan in order to give dfu target
	// time to reboot after a possible goto-dfu command.
	LOGMeshDfuHostDebug("+++ startWaitForTargetReboot");

	setEventCallback(CS_TYPE::EVT_DEVICE_SCANNED, &MeshDfuHost::checkScansForTarget);
	LOGMeshDfuHostDebug("+++ waiting for device scan event from target");

	setTimeoutCallback(&MeshDfuHost::completePhase);
	LOGMeshDfuHostDebug("+++ waiting for timeout");

	return true;
}

void MeshDfuHost::checkScansForTarget(event_t& event) {
	// when the target device is scanned we can cancel the timeout and
	// continue with connecting to it.
	LOGMeshDfuHostDebug("+++ checkScansForTarget received scan");

	if (event.type == CS_TYPE::EVT_DEVICE_SCANNED) {
		scanned_device_t* device = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);

		if (memcmp(device->address, _targetDevice.address, MAC_ADDRESS_LEN) == 0) {
			LOGMeshDfuHostDebug("+++ received a scan from our target");
		}
		else {
			LOGMeshDfuHostDebug("+++ Wait for more scans");
			setEventCallback(CS_TYPE::EVT_DEVICE_SCANNED, &MeshDfuHost::checkScansForTarget);
			return;
		}
	}
	else {
		LOGMeshDfuHostDebug("+++ not a DEVICE_SCANNED event (%u)", event.type);
		setEventCallback(CS_TYPE::EVT_DEVICE_SCANNED, &MeshDfuHost::checkScansForTarget);
		return;
	}

	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completeWaitForTargetReboot() {
	LOGMeshDfuHostDebug("completeWaitForTargetReboot");
	return Phase::ConnectTargetInDfuMode;
}

// ###### ConnectTargetInDfuMode ######
#define _PHASE_ConnectTargetInDfuMode_

bool MeshDfuHost::startConnectTargetInDfuMode() {
	// called both on timeout and after a received scan from target device.
	LOGMeshDfuHostDebug("startConnectTargetInDfuMode");

	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT, &MeshDfuHost::checkDfuTargetConnected);

	setTimeoutCallback(&MeshDfuHost::checkDfuTargetConnected);

	auto status = _bleCentral->connect(_targetDevice);

	LOGMeshDfuHostDebug("+++ waiting for BLE central connect result. returnval: %u", status);

	if (status != ERR_WAIT_FOR_SUCCESS) {
		LOGw("BLE central busy or in wrong state. Expecting timeout to occur");
	}

	return true;
}

void MeshDfuHost::checkDfuTargetConnected(event_t& event) {
	if (event.type != CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT) {
		LOGMeshDfuHostDebug("checkDfuTargetConnected responded to unexpected event.");
		return;
	}
	checkDfuTargetConnected();
}

void MeshDfuHost::checkDfuTargetConnected() {
	// Called on ble_central_connect_result and on timeout, tries reconnecting if necessary.
	// Not waiting for scans as we have recently tried that.

	LOGMeshDfuHostDebug("+++ checkDfuTargetConnected");
	if (!_bleCentral->isConnected()) {
		if (_reconnectionAttemptsLeft > 0) {
			LOGMeshDfuHostDebug(
					"+++ BLE central connection failed, retrying. Attempts left: %d", _reconnectionAttemptsLeft);
			CsMath::Decrease(_reconnectionAttemptsLeft);

			setTimeoutCallback(&MeshDfuHost::restartPhase, 10000);
			return;
		}
		else {
			abort();
			return;
		}
	}

	LOGMeshDfuHostDebug("+++ reconnection successful");

	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completeConnectTargetInDfuMode() {
	LOGMeshDfuHostDebug("+++ completeConnectTargetInDfuMode");
	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
	return Phase::DiscoverDfuCharacteristics;
}

// ###### DiscoverDfuCharacteristics ######
#define _PHASE_DiscoverDfuCharacteristics_

bool MeshDfuHost::startDiscoverDfuCharacteristics() {
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT, &MeshDfuHost::onDiscoveryResult);
	setTimeoutCallback(&MeshDfuHost::completePhase);

	auto status =
			_bleCentral->discoverServices(_meshDfuTransport.getServiceUuids(), _meshDfuTransport.getServiceUuidCount());

	if (status != ERR_WAIT_FOR_SUCCESS) {
		if (_reconnectionAttemptsLeft > 0) {
			CsMath::Decrease(_reconnectionAttemptsLeft);
			LOGw("BLE central busy or in wrong state. Retrying in a few seconds");
			setTimeoutCallback(&MeshDfuHost::restartPhase, 5000);
		}
		else {
			abort();
			return false;
		}
	}

	return true;
}

void MeshDfuHost::onDiscoveryResult(event_t& event) {
	LOGMeshDfuHostDebug("+++ onDiscoveryResult");

	TYPIFY(EVT_BLE_CENTRAL_DISCOVERY_RESULT)* result = CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
	if (*result != ERR_SUCCESS) {
		// early abort on fail.
		abort();
		return;
	}

	if (!_meshDfuTransport.isTargetInDfuMode()) {
		LOGMeshDfuHostDebug("+++ dfu mode verification failed, disconnecting and retrying");
		// we'll need to reconnect as crownstone central

		setTimeoutCallback(&MeshDfuHost::completePhase);
		setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::completePhase);

		_bleCentral->disconnect();
		return;
	}

	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completeDiscoverDfuCharacteristics() {
	LOGMeshDfuHostDebug("+++ completeDiscoverDfuCharacteristics");

	if (_meshDfuTransport.isTargetInDfuMode()) {
		LOGMeshDfuHostDebug("+++ TargetTriggerDfuMode: success, dfu mode verified");
		return Phase::TargetPreparing;
	}

	if (!_triedDfuCommand) {
		LOGMeshDfuHostDebug("+++ TargetTriggerDfuMode retrying");
		_triedDfuCommand = true;
		return Phase::TargetTriggerDfuMode;
	}

	LOGMeshDfuHostDebug("+++ DiscoverDfuCharacteristics failed");
	return Phase::Aborting;
}

// ###### TargetPreparing ######
#define _PHASE_TargetPreparing_

bool MeshDfuHost::startPhaseTargetPreparing() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetPreparing");

	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT, &MeshDfuHost::continuePhaseTargetPreparing);
	setTimeoutCallback(&MeshDfuHost::abort);
	cs_ret_code_t ret = _meshDfuTransport.enableNotifications(true);

	// TODO: retry?

	return ret == ERR_WAIT_FOR_SUCCESS;
}

void MeshDfuHost::continuePhaseTargetPreparing(event_t& event) {
	TYPIFY(EVT_BLE_CENTRAL_WRITE_RESULT)* result = CS_TYPE_CAST(EVT_BLE_CENTRAL_WRITE_RESULT, event.data);

	if (*result != ERR_SUCCESS) {
		LOGMeshDfuHostWarn("failed enabling notifications.");
		abort();
		return;
	}

	setEventCallback(CS_TYPE::EVT_MESH_DFU_TRANSPORT_RESULT, &MeshDfuHost::checkResultPhaseTargetPreparing);
	_meshDfuTransport.prepare();
}

void MeshDfuHost::checkResultPhaseTargetPreparing(event_t& event) {
	TYPIFY(EVT_MESH_DFU_TRANSPORT_RESULT)* result = CS_TYPE_CAST(EVT_MESH_DFU_TRANSPORT_RESULT, event.data);

	switch (*result) {
		case ERR_SUCCESS: {
			LOGMeshDfuHostDebug("check result of target preparing: success");
			completePhase();
			break;
		}
		default: {
			LOGMeshDfuHostWarn("failed to start dfu process: %u", *result);
			abort();
			break;
		}
	}
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetPreparing() {
	LOGMeshDfuHostDebug("+++ completePhaseTargetPreparing");
	return Phase::TargetInitializing;
}
// ###### TargetInitializing ######
#define _PHASE_TargetInitializing_() \
	{}

bool MeshDfuHost::startPhaseTargetInitializing() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetInitializing");
	setEventCallback(CS_TYPE::EVT_MESH_DFU_TRANSPORT_RESPONSE, &MeshDfuHost::targetInitializingCreateCommand);
	_meshDfuTransport._selectCommand();

	return true;
}

void MeshDfuHost::targetInitializingCreateCommand(event_t& event) {
	TYPIFY(EVT_MESH_DFU_TRANSPORT_RESPONSE) result = *CS_TYPE_CAST(EVT_MESH_DFU_TRANSPORT_RESPONSE, event.data);
	LOGMeshDfuHostDebug(
			"response for selectCommand: %u, {max: %u, off: %u, crc: %x}",
			result.result,
			result.max_size,
			result.offset,
			result.crc);

	if (result.result != ERR_SUCCESS || result.offset != 0) {
		LOGMeshDfuHostWarn("dfu select command failed: %u, offset: %u", result.result, result.offset);
		abort();
		return;
	}

	if (_initPacketLen != 0) {

		setEventCallback(CS_TYPE::EVT_MESH_DFU_TRANSPORT_RESULT, &MeshDfuHost::targetInitializingStreamInitPacket);
		_meshDfuTransport._createCommand(_initPacketLen);
	}
	else {
		abort();
	}
}

void MeshDfuHost::targetInitializingStreamInitPacket(event_t& event) {
	TYPIFY(EVT_MESH_DFU_TRANSPORT_RESULT) result = *CS_TYPE_CAST(EVT_MESH_DFU_TRANSPORT_RESULT, event.data);
	LOGMeshDfuHostDebug("response for createCommand: %u", result);

	if (result != ERR_SUCCESS) {
		LOGMeshDfuHostWarn("dfu createCommand failed: %u", result);
		abort();
		return;
	}

	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;

	// setup stream: offset by 4 byte size and 4 byte verification in flash must be skipped
	setStreamState(FirmwareSection::MicroApp, 2 * sizeof(uint32_t), _initPacketLen, 0);

	stream();
}

void MeshDfuHost::targetInitializingExecute(event_t& event) {
	// TODO: this will need to be in the next phase as stream() will completePhase().
	// _meshDfuTransport._execute();
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetInitializing() {
	LOGMeshDfuHostDebug("+++ completePhaseTargetInitializing");
	// TODO
	return Phase::None;
}

// ###### TargetUpdating ######
#define _PHASE_TargetUpdating_

bool MeshDfuHost::startPhaseTargetUpdating() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetUpdating");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetUpdating() {
	LOGMeshDfuHostDebug("+++ completePhaseTargetUpdating");
	// TODO
	return Phase::None;
}

// ###### TargetVerifying ######
#define _PHASE_TargetVerifying_

bool MeshDfuHost::startPhaseTargetVerifying() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetVerifying");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetVerifying() {
	LOGMeshDfuHostDebug("+++ completePhaseTargetVerifying");
	// TODO
	return Phase::None;
}

// ###### Aborting ######
#define _PHASE_Aborting_

bool MeshDfuHost::startPhaseAborting() {
	LOGMeshDfuHostDebug("+++ startPhaseAborting");

	clearEventCallback();
	clearTimeoutCallback();

	// its enough to disconnect crownstone central: that will disconnect
	// ble central even if we didn't use crownstone central for the connection.
	LOGMeshDfuHostDebug("disconnecting crownstone central");
	auto csCentralStatus = _crownstoneCentral->disconnect();

	switch (csCentralStatus) {
		case ERR_WAIT_FOR_SUCCESS: {
			LOGMeshDfuHostDebug("wait for disconnect event");
			setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::completePhase);
			setTimeoutCallback(&MeshDfuHost::completePhase);
			break;
		}
		default: {
			LOGMeshDfuHostDebug("disconnect failed, finishing abort next tick");
			setEventCallback(CS_TYPE::EVT_TICK, &MeshDfuHost::completePhase);
			break;
		}
	}

	return true;
}

// ###### Aborting ######

MeshDfuHost::Phase MeshDfuHost::completePhaseAborting() {
	LOGMeshDfuHostDebug("+++ completePhaseAborting");

	clearEventCallback();
	clearTimeoutCallback();

	return Phase::Idle;
}

// ------------------------------------------------------------------------------------
// --------------------------- phase administration methods ---------------------------
// ------------------------------------------------------------------------------------

#define ________PHASE_ADMINISTRATION________

bool MeshDfuHost::startPhase(Phase phase) {
	LOGMeshDfuHostDebug("*************************************************");
	LOGMeshDfuHostDebug("+++ Starting phase %s", phaseName(phase));
	_phaseCurrent    = phase;
	_phaseOnComplete = Phase::None;

	bool success = false;

	switch (phase) {
		case Phase::Idle: {
			success = startPhaseIdle();
			break;
		}
		case Phase::TargetTriggerDfuMode: {
			success = startPhaseTargetTriggerDfuMode();
			break;
		}
		case Phase::WaitForTargetReboot: {
			success = startWaitForTargetReboot();
			break;
		}
		case Phase::ConnectTargetInDfuMode: {
			success = startConnectTargetInDfuMode();
			break;
		}
		case Phase::DiscoverDfuCharacteristics: {
			success = startDiscoverDfuCharacteristics();
			break;
		}
		case Phase::TargetPreparing: {
			success = startPhaseTargetPreparing();
			break;
		}
		case Phase::TargetInitializing: {
			success = startPhaseTargetInitializing();
			break;
		}
		case Phase::TargetUpdating: {
			success = startPhaseTargetUpdating();
			break;
		}
		case Phase::TargetVerifying: {
			success = startPhaseTargetVerifying();
			break;
		}
		case Phase::Aborting: {
			success = startPhaseAborting();
			break;
		}
		case Phase::None: {
			success = false;  // None cannot be entered. It indicates reboot.
			break;
		}
	}

	if (!success) {
		LOGMeshDfuHostDebug("+++ Failed to start phase %s, aborting", phaseName(phase));
		abort();
		return false;
	}

	return success;
}

void MeshDfuHost::completePhase() {
	LOGMeshDfuHostDebug("+++ Completeing phase %s", phaseName(_phaseCurrent));

	Phase phaseNext = Phase::None;

	switch (_phaseCurrent) {
		case Phase::Idle: {
			// idle doesn't have a 'next phase'. To start another phase call startPhase instead.
			phaseNext = Phase::Idle;
			break;
		}
		case Phase::TargetTriggerDfuMode: {
			phaseNext = completePhaseTargetTriggerDfuMode();
			break;
		}
		case Phase::WaitForTargetReboot: {
			phaseNext = completeWaitForTargetReboot();
			break;
		}
		case Phase::ConnectTargetInDfuMode: {
			phaseNext = completeConnectTargetInDfuMode();
			break;
		}
		case Phase::DiscoverDfuCharacteristics: {
			phaseNext = completeDiscoverDfuCharacteristics();
			break;
		}
		case Phase::TargetPreparing: {
			phaseNext = completePhaseTargetPreparing();
			break;
		}
		case Phase::TargetInitializing: {
			phaseNext = completePhaseTargetInitializing();
			break;
		}
		case Phase::TargetUpdating: {
			break;  // TODO: add callbacks for completePhaseX
		}
		case Phase::TargetVerifying: {
			break;  // TODO: add callbacks for completePhaseX
		}
		case Phase::Aborting: {
			phaseNext = completePhaseAborting();
			break;
		}
		case Phase::None: break;
	}

	if (_phaseOnComplete != Phase::None) {
		LOGMeshDfuHostInfo("+++ DFU phase next progress was overriden by user");
		phaseNext = _phaseOnComplete;
	}

	_phaseCurrent = phaseNext;
	setEventCallback(CS_TYPE::EVT_TICK, &MeshDfuHost::startPhase);
}

void MeshDfuHost::startPhase(event_t& event) {
	startPhase(_phaseCurrent);
}

void MeshDfuHost::completePhase(event_t& event) {
	completePhase();
}

void MeshDfuHost::restartPhase() {
	LOGMeshDfuHostDebug("Restarting phase %s", phaseName(_phaseCurrent));
	startPhase(_phaseCurrent);
}

void MeshDfuHost::abort() {
	startPhase(Phase::Aborting);
}

// ------------------------------------------------------------------------------------
// -------------------------- callback implementation methods -------------------------
// ------------------------------------------------------------------------------------

#define ________CALLBACK_IMPLEMENTATION________

bool MeshDfuHost::setEventCallback(CS_TYPE evtToWaitOn, EventCallback callback) {
	bool overridden = _onExpectedEvent != nullptr;

	_expectedEvent   = evtToWaitOn;
	_onExpectedEvent = callback;

	return overridden;
}

bool MeshDfuHost::setTimeoutCallback(TimeoutCallback onTimeout, uint32_t timeoutMs) {
	bool overriden = _onTimeout != nullptr;

	_onTimeout = onTimeout;
	_timeOutRoutine.startSingleMs(timeoutMs);

	return overriden;
}

void MeshDfuHost::clearEventCallback() {
	_onExpectedEvent = nullptr;
}

void MeshDfuHost::clearTimeoutCallback() {
	_onTimeout = nullptr;
	_timeOutRoutine.pause();
}

void MeshDfuHost::onEventCallbackTimeOut() {
	LOGMeshDfuHostInfo("+++ event callback timed out, calling _onTimeout");
	if (_onTimeout != nullptr) {
		(this->*_onTimeout)();
	}
}

// -------------------------------------------------------------------------------------
// --------------------------------------- utils ---------------------------------------
// -------------------------------------------------------------------------------------

#define ________UTILS________

bool MeshDfuHost::isInitialized() {
	return _bleCentral != nullptr && _crownstoneCentral != nullptr;
}

bool MeshDfuHost::ableToLaunchDfu() {
	return haveInitPacket() && isDfuProcessIdle();
}

bool MeshDfuHost::haveInitPacket() {
	switch (_initPacketLen) {
		case 0: {
			[[fallthrough]];
		}
		case 0xffffffff: {
			return false;
		}
		default: {
			return true;
		}
	}
}

void MeshDfuHost::loadInitPacketLen() {
	uint32_t nrfCode = _firmwareReader->read(0, sizeof(_initPacketLen), &_initPacketLen, FirmwareSection::MicroApp);

	if (nrfCode != ERR_SUCCESS) {
		LOGMeshDfuHostWarn("init packet couldnt be read. Status: %u", nrfCode);
		_initPacketLen = 0;
	}

	if (_initPacketLen == 0 || _initPacketLen == 0xffffffff) {
		LOGMeshDfuHostWarn("init packet seems to be missing, length is zero or -1: %u", _initPacketLen);
		_initPacketLen = 0;
	}
	else {
		LOGMeshDfuHostDebug("found init packet, len: %u", _initPacketLen);
	}
}

bool MeshDfuHost::isDfuProcessIdle() {
	// if not waiting on any updates, we must be done.
	bool idle = _phaseCurrent == Phase::Idle;

	if (idle) {
		if (_onTimeout != nullptr || _onExpectedEvent != nullptr) {
			LOGw("meshDfuHost idle but callbacks non-null clearing.");
			clearEventCallback();
			clearTimeoutCallback();
		}
	}

	return idle;
}

void MeshDfuHost::reset() {
	LOGMeshDfuHostDebug("resetting component");
	_triedDfuCommand          = false;
	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
	clearEventCallback();
	clearTimeoutCallback();
	clearStreamState();
}