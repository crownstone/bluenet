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

bool MeshDfuHost::startPhaseTargetTriggerDfuMode() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetTriggerDfuMode");
	setEventCallback(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::sendDfuCommand);

	auto status = _crownstoneCentral->connect(_targetDevice);

	LOGMeshDfuHostDebug("+++ waiting for cs central connect result");
	return status == ERR_WAIT_FOR_SUCCESS;
}

void MeshDfuHost::sendDfuCommand(event_t& event) {
	LOGMeshDfuHostDebug("+++ sendDfuCommand");
	if(!_isCrownstoneCentralConnected) {
		if(_reconnectionAttemptsLeft > 0) {
			LOGMeshDfuHostDebug("+++ Crownstone central connection failed, retrying. Attempts left: %d", _reconnectionAttemptsLeft);
			setEventCallback(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::sendDfuCommand);

			_reconnectionAttemptsLeft -= 1;
			_crownstoneCentral->connect(_targetDevice);

			LOGMeshDfuHostDebug("+++ waiting for crownstone central connect result");
			return;
		} else {
			startPhase(Phase::Aborting);
			return;
		}
	}

	// expect a disconnection after writing the control command
	// this needs to be the BLE_CENTRAL event because CrownstoneCentral may not have an
	// active operation.
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED, &MeshDfuHost::verifyDisconnectAfterDfu);

	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
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
	return Phase::ConnectTargetInDfuMode;
}

// ###### ConnectTargetInDfuMode ######

bool MeshDfuHost::startConnectTargetInDfuMode() {
	setEventCallback(
				CS_TYPE::EVT_DEVICE_SCANNED,
				&MeshDfuHost::connectTargetInDfuMode);
	LOGMeshDfuHostDebug("+++ waiting for device scan event from target");
	return true;
}

void MeshDfuHost::connectTargetInDfuMode(event_t& event) {
	LOGMeshDfuHostDebug("+++ startConnectTargetInDfuMode");
	if(event.type == CS_TYPE::EVT_DEVICE_SCANNED) {
		scanned_device_t* device = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);
		if(memcmp(device->address, _targetDevice.address, MAC_ADDRESS_LEN) == 0) {
			LOGMeshDfuHostDebug("+++ received a scan from our target");

		} else {
			LOGMeshDfuHostDebug("+++ Wait for more scans");
			setEventCallback(
							CS_TYPE::EVT_DEVICE_SCANNED,
							&MeshDfuHost::connectTargetInDfuMode);
			return;
		}
	} else {
		LOGMeshDfuHostDebug("+++ not a DEVICE_SCANNED event (%u)", event.type);
		setEventCallback(
						CS_TYPE::EVT_DEVICE_SCANNED,
						&MeshDfuHost::connectTargetInDfuMode);
		return;
	}

	// TODO: also respond on time-out

	setEventCallback(
			CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT,
			&MeshDfuHost::verifyDfuMode,
			MeshDfuConstants::DfuHostSettings::ConnectionTimeoutMs,
			Phase::ConnectTargetInDfuMode);

	auto status = _bleCentral->connect(_targetDevice);

	LOGMeshDfuHostDebug("+++ waiting for BLE central connect result. Status: %u", status);
	if(status != ERR_WAIT_FOR_SUCCESS) {
		LOGe("TODO: what to do if ble central is busy or in error state?");
	}
}

void MeshDfuHost::verifyDfuMode(event_t& event) {
	LOGMeshDfuHostDebug("+++ reconnectAfterDfuCommand");

	if(!_bleCentral->isConnected()) {
		if(_reconnectionAttemptsLeft > 0) {
			LOGMeshDfuHostDebug("+++ BLE central connection failed, retrying. Attempts left: %d", _reconnectionAttemptsLeft);
			setEventCallback(
					CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT,
					&MeshDfuHost::verifyDfuMode,
					MeshDfuConstants::DfuHostSettings::ConnectionTimeoutMs,
					Phase::ConnectTargetInDfuMode);

			_reconnectionAttemptsLeft -= 1;
			cs_ret_code_t res = _bleCentral->connect(_targetDevice);

			if(res != ERR_WAIT_FOR_SUCCESS) {
				LOGMeshDfuHostDebug("+++ BLE central connect result not OK: %d. Retry after timeout.", res);
			}
			return;
		} else {
			startPhase(Phase::Aborting);
			return;
		}
	}

	LOGMeshDfuHostDebug("TODO: check characteristics of connected device and return abort if wrong, or Phase::TargetPreparing else.");

	completePhase();
}


MeshDfuHost::Phase MeshDfuHost::completeConnectTargetInDfuMode() {
	LOGMeshDfuHostDebug("+++ completeConnectTargetInDfuMode");
	return Phase::TargetPreparing;
}


// ###### TargetPreparing ######

bool MeshDfuHost::startPhaseTargetPreparing() {
	LOGMeshDfuHostDebug("+++ startPhaseTargetPreparing");
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
	setEventCallback(CS_TYPE::EVT_TICK,&MeshDfuHost::aborting);
	// TODO: disconnect
	return true;
}

void MeshDfuHost::aborting(event_t& event) {
	completePhase();
}

MeshDfuHost::Phase MeshDfuHost::completePhaseAborting() {
	LOGMeshDfuHostDebug("+++ completePhaseAborting");
	// TODO: clearEventCallback, set phases to none etc.
	return Phase::Idle;
}


// ---------------------------- progress related callbacks ----------------------------

bool MeshDfuHost::startPhase(Phase phase) {
	LOGMeshDfuHostDebug("+++ Starting phase %d", phase);
	bool success = false;

	switch (phase) {
		case Phase::Idle: {
			success = true;
			break;
		}
		case Phase::TargetTriggerDfuMode: {
			success = startPhaseTargetTriggerDfuMode();
			break;
		}
		case Phase::ConnectTargetInDfuMode: {
			success = startConnectTargetInDfuMode();
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
		LOGMeshDfuHostDebug("+++ Failed to start phase %d, aborting", static_cast<uint8_t>(phase));
		startPhase(Phase::Aborting);
		return false;
	}

	return success;
}

void MeshDfuHost::completePhase() {
	LOGMeshDfuHostDebug("+++ Completeing phase %d", _phaseCurrent);

	Phase phaseNext = Phase::None;

	// TODO: add callbacks for completePhaseX

	switch(_phaseCurrent) {
		case Phase::Idle: break;
		case Phase::TargetTriggerDfuMode: {
			phaseNext = completePhaseTargetTriggerDfuMode();
			break;
		}
		case Phase::ConnectTargetInDfuMode: {
			phaseNext = completeConnectTargetInDfuMode();
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

bool MeshDfuHost::setEventCallback(CS_TYPE evtToWaitOn, EventCallback callback, uint32_t timeoutMs, Phase onTimeout) {
	bool overridden = _expectedEventCallback != nullptr;

	_expectedEvent         = evtToWaitOn;
	_expectedEventCallback = callback;
	_phaseOnTimeout = onTimeout;

	resetTimeout(timeoutMs);

	return overridden;
}
void MeshDfuHost::clearEventCallback() {
	_expectedEventCallback = nullptr;
}


void MeshDfuHost::onEventCallbackTimeOut() {
	LOGMeshDfuHostInfo("+++ event callback timed out, starting _phaseOnTimeout");
	startPhase(_phaseOnTimeout);
}

void MeshDfuHost::resetTimeout(uint32_t timeoutMs) {
	_timeOutRoutine.startSingleMs(timeoutMs);
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

	_timeOutRoutine._action =
			[this]() {
				onEventCallbackTimeOut();
				// (timeoutroutine is intended as single shot but have to return a delay value)
				return Coroutine::delayS(10);
			};
	_timeOutRoutine.pause();

	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;

	LOGMeshDfuHostDebug("MeshDfuHost init successful");
	return ERR_SUCCESS;
}

bool MeshDfuHost::haveInitPacket() {
	return true; // TODO actually check
}


bool MeshDfuHost::ableToLaunchDfu() {
	return haveInitPacket() && dfuProcessIdle();
}

bool MeshDfuHost::dfuProcessIdle() {
	// if not waiting on any updates, we must be done.
	return _expectedEventCallback == nullptr;
//	return _phaseCurrent == Phase::Idle;
}

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
	if (_expectedEventCallback != nullptr && event.type == _expectedEvent) {
		// cancel timeout
		_timeOutRoutine.pause();

		// create local copy
		auto eventCallback = _expectedEventCallback;

		// clear expected event
		_expectedEventCallback = nullptr;
		(this->*eventCallback)(event);
	}

	_timeOutRoutine.handleEvent(event);

	if(event.type == CS_TYPE::EVT_TICK) {
		if (ticks_until_start % 10 == 1) {
			LOGMeshDfuHostDebug("tick counting: %d ", ticks_until_start);
		}
		if(CsMath::Decrease(ticks_until_start) == 1){
			LOGMeshDfuHostDebug("starting dfu at ticks left : %d ", ticks_until_start);
			copyFirmwareTo(_debugTarget);
		}

//		uint32_t tickCount = *reinterpret_cast<TYPIFY(EVT_TICK)*>(event.data);
//		if (tickCount % 10 == 0) {
//			LOGMeshDfuHostDebug("_phaseCurrent: %d, _phaseOnComplete %d, _phaseOnTimeout %d",
//					_phaseCurrent,
//					_phaseOnComplete,
//					_phaseOnTimeout);
//			LOGMeshDfuHostDebug("timeoutRoutine mode %d, nexttick %u, tickcount %u",
//					_timeOutRoutine.getMode(),
//					_timeOutRoutine.getNextTick(),
//					tickCount);
//		}
	}
}
