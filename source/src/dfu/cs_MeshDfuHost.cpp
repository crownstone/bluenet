/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <dfu/cs_MeshDfuHost.h>
#include <dfu/cs_MeshDfuConstants.h>
#include <logging/cs_Logger.h>

#define LOGMeshDfuHostDebug LOGd
#define LOGMeshDfuHostInfo LOGi

bool MeshDfuHost::copyFirmwareTo(device_address_t target) {
	LOGMeshDfuHostDebug("+++ Copy firmware to target");
	if(!isInitialized()) {
		return init() == ERR_SUCCESS;
	}

	if (!ableToLaunchDfu()) {
		return false;
	}

	_targetDevice = target;

	return startPhase(Phase::TargetTriggerDfuMode);
}

// -------------------------------------------------------------------------------------
// ------------------------------- phase implementations -------------------------------
// -------------------------------------------------------------------------------------


// ###### TargetTriggerDfuMode ######

bool MeshDfuHost::startPhaseIdle() {
	reset();
	return true;
}


// ###### TargetTriggerDfuMode ######

bool MeshDfuHost::startPhaseTargetTriggerDfuMode() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetTriggerDfuMode");

	if (_reconnectionAttemptsLeft > 0) {
		CsMath::Decrease(_reconnectionAttemptsLeft);

		setEventCallback(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::sendDfuCommand);
		setTimeoutCallback(&MeshDfuHost::restartPhase);

		auto status = _crownstoneCentral->connect(_targetDevice);

		switch (status) {
			case ERR_BUSY: {
				LOGMeshDfuHostDebug("+++ CrownstoneCentral busy. Retrying after timeout. %u attempts left",
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
	if(!_isCrownstoneCentralConnected) {
		LOGMeshDfuHostDebug("+++ crownstone central not connected.");
		// this will abort if _reconnectionAttemptsLeft reaches zero.
		restartPhase();
	}

	LOGMeshDfuHostDebug("+++ sendDfuCommand");

	// expect a disconnection after writing the control command
	// this needs to be the BLE_CENTRAL event because CrownstoneCentral may not have an
	// active operation.
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::verifyDisconnectAfterDfu);
	setTimeoutCallback(&MeshDfuHost::abort);

	_crownstoneCentral->write(CommandHandlerTypes::CTRL_CMD_GOTO_DFU, nullptr, 0);

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

bool MeshDfuHost::startWaitForTargetReboot() {
	// start this phase waiting for a scan in order to give dfu target
	// time to reboot after a possible goto-dfu command.
	LOGMeshDfuHostDebug("+++ startWaitForTargetReboot");

	setEventCallback(
				CS_TYPE::EVT_DEVICE_SCANNED,
				&MeshDfuHost::checkScansForTarget);
	LOGMeshDfuHostDebug("+++ waiting for device scan event from target");

	setTimeoutCallback(&MeshDfuHost::completePhase);
	LOGMeshDfuHostDebug("+++ waiting for timeout");

	return true;
}

void MeshDfuHost::checkScansForTarget(event_t& event) {
	// when the target device is scanned we can cancel the timeout and
	// continue with connecting to it.
	LOGMeshDfuHostDebug("+++ checkScansForTarget received scan");

	if(event.type == CS_TYPE::EVT_DEVICE_SCANNED) {
		scanned_device_t* device = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);

		if(memcmp(device->address, _targetDevice.address, MAC_ADDRESS_LEN) == 0) {
			LOGMeshDfuHostDebug("+++ received a scan from our target");

		} else {
			LOGMeshDfuHostDebug("+++ Wait for more scans");
			setEventCallback(CS_TYPE::EVT_DEVICE_SCANNED,
							&MeshDfuHost::checkScansForTarget);
			return;
		}
	} else {
		LOGMeshDfuHostDebug("+++ not a DEVICE_SCANNED event (%u)", event.type);
		setEventCallback(CS_TYPE::EVT_DEVICE_SCANNED,
						&MeshDfuHost::checkScansForTarget);
		return;
	}

	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completeWaitForTargetReboot() {
	LOGMeshDfuHostDebug("completeWaitForTargetReboot");
	return Phase::ConnectTargetInDfuMode;
}

// ###### ConnectTargetInDfuMode ######

bool MeshDfuHost::startConnectTargetInDfuMode() {
	// called both on timeout and after a received scan from target device.
	LOGMeshDfuHostDebug("startConnectTargetInDfuMode");

	setEventCallback(
			CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT,
			&MeshDfuHost::checkDfuTargetConnected);

	setTimeoutCallback(&MeshDfuHost::checkDfuTargetConnected);

	auto status = _bleCentral->connect(_targetDevice);

	LOGMeshDfuHostDebug("+++ waiting for BLE central connect result. Status: %u", status);

	if(status != ERR_WAIT_FOR_SUCCESS) {
		LOGw("BLE central busy or in wrong state. Expecting timeout to occur");
	}

	return true;
}

void MeshDfuHost::checkDfuTargetConnected(event_t& event) {
	if(event.type != CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT) {
		LOGMeshDfuHostDebug("checkDfuTargetConnected responded to unexpected event.");
		return;
	}
	checkDfuTargetConnected();
}

void MeshDfuHost::checkDfuTargetConnected() {
	// Called on ble_central_connect_result and on timeout, tries reconnecting if necessary.
	// Not waiting for scans as we have recently tried that.

	LOGMeshDfuHostDebug("+++ checkDfuTargetConnected");
	if(!_bleCentral->isConnected()) {
		if(_reconnectionAttemptsLeft > 0) {
			LOGMeshDfuHostDebug("+++ BLE central connection failed, retrying. Attempts left: %d", _reconnectionAttemptsLeft);
			CsMath::Decrease(_reconnectionAttemptsLeft);

			restartPhase();
		} else {
			abort();
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

bool MeshDfuHost::startDiscoverDfuCharacteristics() {
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT, &MeshDfuHost::onDiscoveryResult);
	setTimeoutCallback(&MeshDfuHost::completePhase);

	auto status =_bleCentral->discoverServices(
			_meshDfuTransport.getServiceUuids(),
			_meshDfuTransport.getServiceUuidCount());

	if(status != ERR_WAIT_FOR_SUCCESS) {
		if(_reconnectionAttemptsLeft > 0){
			CsMath::Decrease(_reconnectionAttemptsLeft);
			LOGw("BLE central busy or in wrong state. Retrying in a few seconds");
			setTimeoutCallback(&MeshDfuHost::restartPhase, 5000);
		} else {
			abort();
		}
	}

	return true;
}

void MeshDfuHost::onDiscoveryResult(event_t& event) {
	LOGMeshDfuHostDebug("+++ onDiscoveryResult");

	TYPIFY(EVT_BLE_CENTRAL_DISCOVERY_RESULT)* result = CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
	if(*result != ERR_SUCCESS){
		// early abort on fail.
		abort();
		return;
	}

	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completeDiscoverDfuCharacteristics() {
	LOGMeshDfuHostDebug("+++ completeDiscoverDfuCharacteristics");

	if(_meshDfuTransport.isTargetInDfuMode()) {
		LOGMeshDfuHostDebug("+++ dfu mode verified");
		return Phase::TargetPreparing;
	}

	if(!_triedDfuCommand) {
		_triedDfuCommand = true;
		return Phase::TargetTriggerDfuMode;
	}

	LOGMeshDfuHostDebug("+++ dfu mode verification failed");
	return Phase::Aborting;
}

// ###### TargetPreparing ######

bool MeshDfuHost::startPhaseTargetPreparing() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetPreparing");
	_meshDfuTransport.prepare();
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetPreparing() {
	LOGMeshDfuHostDebug("+++ completePhaseTargetPreparing");
	// TODO
	return Phase::None;
}
// ###### TargetInitializing ######

bool MeshDfuHost::startPhaseTargetInitializing() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetInitializing");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetInitializing() {
	LOGMeshDfuHostDebug("+++ completePhaseTargetInitializing");
	// TODO
	return Phase::None;
}
// ###### TargetUpdating ######

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
// ###### TargetTriggerDfuMode ######

bool MeshDfuHost::startPhaseAborting() {
	LOGMeshDfuHostDebug("+++ startPhaseAborting");

	clearEventCallback();
	clearTimeoutCallback();

	// its enough to disconnect crownstone central: that will disconnect
	// ble central even if we didn't use crownstone central for the connection.
	auto csCentralStatus = _crownstoneCentral->disconnect();

	switch(csCentralStatus){
		case ERR_WAIT_FOR_SUCCESS: {
			setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::aborting);
			setTimeoutCallback(&MeshDfuHost::completePhase);
			break;
		}
		default: {
			setEventCallback(CS_TYPE::EVT_TICK,&MeshDfuHost::aborting);
			break;
		}
	}

	return true;
}

void MeshDfuHost::aborting(event_t& event) {
	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completePhaseAborting() {
	LOGMeshDfuHostDebug("+++ completePhaseAborting");

	clearEventCallback();
	clearTimeoutCallback();

	return Phase::Idle;
}



// ---------------------------- progress related callbacks ----------------------------

bool MeshDfuHost::startPhase(Phase phase) {
	LOGMeshDfuHostDebug("+++ Starting phase %s", phaseName(phase));
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
			success = false; // None cannot be entered. It indicates reboot.
			break;
		}
	}

	if(success) {
		_phaseCurrent = phase;
		_phaseOnComplete = Phase::None;
	} else {
		LOGMeshDfuHostDebug("+++ Failed to start phase %s, aborting", phaseName(phase));
		abort();
		return false;
	}

	return success;
}

void MeshDfuHost::completePhase() {
	LOGMeshDfuHostDebug("+++ Completeing phase %s", phaseName(_phaseCurrent));

	Phase phaseNext = Phase::None;

	// TODO: add callbacks for completePhaseX

	switch(_phaseCurrent) {
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
		case Phase::TargetPreparing: break;
		case Phase::TargetInitializing: break;
		case Phase::TargetUpdating: break;
		case Phase::TargetVerifying: break;
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

	startPhase(phaseNext);
}

void MeshDfuHost::restartPhase() {
	LOGMeshDfuHostDebug("Restarting phase %s", phaseName(_phaseCurrent));
	startPhase(_phaseCurrent);
}

void MeshDfuHost::abort() {
	startPhase(Phase::Aborting);
}

// ---------- callbacks ----------

bool MeshDfuHost::setEventCallback(CS_TYPE evtToWaitOn, ExpectedEventCallback callback) {
	bool overridden = _onExpectedEvent != nullptr;

	_expectedEvent         = evtToWaitOn;
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
}

void MeshDfuHost::onEventCallbackTimeOut() {
	LOGMeshDfuHostInfo("+++ event callback timed out, calling _onTimeout");
	if(_onTimeout != nullptr) {
		(this->*_onTimeout)();
	}
}

// --------------------------------------- utils ---------------------------------------

bool MeshDfuHost::isInitialized() {
	return _bleCentral != nullptr && _crownstoneCentral != nullptr;
}

cs_ret_code_t MeshDfuHost::init() {
	LOGMeshDfuHostDebug("init");

	_bleCentral = &BleCentral::getInstance();
	_crownstoneCentral = getComponent<CrownstoneCentral>();

	if (!_listening) {
		LOGMeshDfuHostDebug("Start listening");
		listen();
		_listening = true;
	}

	if(!isInitialized()) {
		LOGMeshDfuHostDebug("MeshDfuHost failed to initialize, blecentral: %0x, cronwstonecentral: %0x", _bleCentral, _crownstoneCentral);
		return ERR_NOT_INITIALIZED;
	}
	if(!haveInitPacket()) {
		LOGMeshDfuHostDebug("MeshDfuHost no init packet available");
		return ERR_NOT_AVAILABLE;
	}

	auto result = _meshDfuTransport.init();
	if(result != ERR_SUCCESS) {
		return result;
	}

	_timeOutRoutine._action =
			[this]() {
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

bool MeshDfuHost::haveInitPacket() {
	return true; // TODO actually check
}


bool MeshDfuHost::ableToLaunchDfu() {
	return haveInitPacket() && isDfuProcessIdle();
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
	LOGMeshDfuHostDebug("resetting");
	_triedDfuCommand          = false;
	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
	clearEventCallback();
	clearTimeoutCallback();
}

// ------------------- event handling ---------------------------

void MeshDfuHost::handleEvent(event_t& event) {

	// connection events always need to update their local _isConnected prior to any event callback handling.
	switch(event.type){
		case CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT : {
			cs_ret_code_t* connectResult = CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
			_isCrownstoneCentralConnected = *connectResult == ERR_SUCCESS;

			LOGMeshDfuHostDebug("Crownstone central connect result received: result %d. Isconnected: %d",
					*connectResult,
					_isCrownstoneCentralConnected);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			cs_ret_code_t* connectResult = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			LOGMeshDfuHostDebug("BLE central connect result received: result %d. Isconnected: %d",
					*connectResult,
					_bleCentral->isConnected());
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

	if(event.type == CS_TYPE::EVT_TICK) {
		if (ticks_until_start % 10 == 1) {
			LOGMeshDfuHostDebug("tick counting: %u ", ticks_until_start);
		}
		if(CsMath::Decrease(ticks_until_start) == 1){
			LOGMeshDfuHostDebug("starting dfu at ticks left : %u ", ticks_until_start);
			copyFirmwareTo(_debugTarget);
		}
	}
}
