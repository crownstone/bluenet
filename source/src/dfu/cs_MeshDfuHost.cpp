/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <dfu/cs_MeshDfuHost.h>
#include <dfu/cs_MeshDfuConstants.h>
#include <logging/cs_Logger.h>

#define LogMeshDfuHostDebug LOGd
#define LogMeshDfuHostInfo LOGi

bool MeshDfuHost::copyFirmwareTo(device_address_t target) {
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
	LogMeshDfuHostDebug("startPhaseTargetTriggerDfuMode");

	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
	_crownstoneCentral->connect(_targetDevice);

	LogMeshDfuHostDebug("waiting for cs central connect result");
	return waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::sendDfuCommand);
}

void MeshDfuHost::sendDfuCommand(event_t& event) {
	if(!_isCrownstoneCentralConnected) {
		if(_reconnectionAttemptsLeft > 0) {
			LogMeshDfuHostDebug("connection failed, retrying");
			_reconnectionAttemptsLeft -= 1;

			_crownstoneCentral->connect(_targetDevice);
			waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::sendDfuCommand);

			LogMeshDfuHostDebug("waiting for cs central connect result");
		} else {
			stopDfu();
		}
	}

	_reconnectionAttemptsLeft = MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts;
	_crownstoneCentral->write(CommandHandlerTypes::CTRL_CMD_GOTO_DFU, nullptr, 0);
	waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::reconnectAfterDfuCommand);

	LogMeshDfuHostDebug("waiting for disconnect after command goto dfu");
}

void MeshDfuHost::reconnectAfterDfuCommand(event_t& event) {
	if (_isCrownstoneCentralConnected) {
		LogMeshDfuHostDebug("incorrect state, device is still connected after dfu command.");
		waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, &MeshDfuHost::reconnectAfterDfuCommand);
		LogMeshDfuHostDebug("waiting for disconnect after command goto dfu");
		return;
	}

	if(!_isBleCentralConnected) {
		if(_reconnectionAttemptsLeft > 0) {
			_reconnectionAttemptsLeft -= 1;

			_bleCentral->connect(_targetDevice);
			waitForEvent(CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT, &MeshDfuHost::reconnectAfterDfuCommand);
		} else {
			stopDfu();
		}
	}

	LogMeshDfuHostDebug("waiting for cs central connect result");
}

void MeshDfuHost::verifyDfuMode(event_t& event) {
	LogMeshDfuHostDebug("verify the dfu mode (TODO)");
	// TODO
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetTriggerDfuMode() {
	LogMeshDfuHostDebug("completePhaseTargetTriggerDfuMode");
	// TODO
	return Phase::None;
}





// ###### TargetPreparing ######

bool startPhaseTargetPreparing() {
	LogMeshDfuHostDebug("startPhaseTargetPreparing");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetPreparing() {
	LogMeshDfuHostDebug("completePhaseTargetPreparing");
	// TODO
	return Phase::None;
}
// ###### TargetInitializing ######

bool startPhaseTargetInitializing() {
	LogMeshDfuHostDebug("startPhaseTargetInitializing");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetInitializing() {
	LogMeshDfuHostDebug("completePhaseTargetInitializing");
	// TODO
	return Phase::None;
}
// ###### TargetUpdating ######

bool startPhaseTargetUpdating() {
	LogMeshDfuHostDebug("startPhaseTargetUpdating");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetUpdating() {
	LogMeshDfuHostDebug("completePhaseTargetUpdating");
	// TODO
	return Phase::None;
}
// ###### TargetVerifying ######

bool startPhaseTargetVerifying() {
	LogMeshDfuHostDebug("startPhaseTargetVerifying");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseTargetVerifying() {
	LogMeshDfuHostDebug("completePhaseTargetVerifying");
	// TODO
	return Phase::None;
}
// ###### TargetTriggerDfuMode ######

bool startPhaseAborting() {
	LogMeshDfuHostDebug("startPhaseAborting");
	// TODO
	return false;
}

MeshDfuHost::Phase MeshDfuHost::completePhaseAborting() {
	LogMeshDfuHostDebug("completePhaseAborting");
	// TODO
	return Phase::None;
}

// ----------------------





// ----------------------------------------------------------------------------------


//void MeshDfuHost::finalizeTargetTriggerDfuMode() {
//	// check dfu mode success.
//	completePhaseAndGoto(Phase::TargetPreparing);
//}
//
//// Phase::TargetPreparing
//
//void MeshDfuHost::startPhaseTargetPreparing() {
//
//	// __send_prn();
//}
//
//void MeshDfuHost::finalizeTargetPreparing() {
//	// inspect result
//	completePhaseAndGoto(Phase::TargetInitializing);
//}
//
//void MeshDfuHost::startPhaseTargetInitializing() {}
//void MeshDfuHost::startPhaseTargetUpdating() {}
//void MeshDfuHost::startPhaseTargetVerifying() {}
//
//void MeshDfuHost::startPhaseAborting() {
//	// invalidate _targetDevice (in flash)
//}

// ---------------------------- progress related callbacks ----------------------------

bool MeshDfuHost::startPhase(Phase phase) {
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
		_phaseNextOverride = Phase::None;
	}

	return success;
}

void MeshDfuHost::completePhase() {
	Phase phaseNext = Phase::None;

	// TODO: add callbacks for completePhaseX

	switch(_phaseCurrent) {
		case Phase::Idle: break;
		case Phase::TargetTriggerDfuMode: break;
		case Phase::TargetPreparing: break;
		case Phase::TargetInitializing: break;
		case Phase::TargetUpdating: break;
		case Phase::TargetVerifying: break;
		case Phase::Aborting: break;
		case Phase::None: break;
	}

	if (_phaseNextOverride != Phase::None) {
		LogMeshDfuHostInfo("DFU phase next progress was overriden by user");
		startPhase(_phaseNextOverride);
	}

	startPhase(phaseNext);
}

bool MeshDfuHost::waitForEvent(CS_TYPE evtToWaitOn, PhaseCallback callback) {
	if (_expectedEventCallback != nullptr) {
		return false;
	}

	_expectedEvent         = evtToWaitOn;
	_expectedEventCallback = callback;

	return true;
}

// --------------------------------------- utils ---------------------------------------

bool MeshDfuHost::isInitialized() {
	return _bleCentral != nullptr && _crownstoneCentral != nullptr;
}

cs_ret_code_t MeshDfuHost::init() {
	_bleCentral = &BleCentral::getInstance();
	_crownstoneCentral = getComponent<CrownstoneCentral>();

	if (!_listening) {
		listen();
		_listening = true;
	}

	if(!isInitialized()) {
		LogMeshDfuHostDebug("MeshDfuHost failed to initialize");
		return ERR_NOT_INITIALIZED;
	}
	if(!haveInitPacket()) {
		LogMeshDfuHostDebug("MeshDfuHost no init packet available");
		return ERR_NOT_AVAILABLE;
	}

	LogMeshDfuHostDebug("MeshDfuHost init successful");
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

	// connection events always need to update their local _isConnected states
	switch(event.type){
		case CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT : {
			cs_ret_code_t* connectResult = CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
			_isCrownstoneCentralConnected = *connectResult == ERR_SUCCESS;
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			cs_ret_code_t* connectResult = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			_isBleCentralConnected = *connectResult == ERR_SUCCESS;
			break;
		}
		default: {
			break;
		}
	}


	// if a callback is waiting on this event call it back.
	if (_expectedEventCallback != nullptr && event.type == _expectedEvent) {
		// create local copy
		auto eventCallback = _expectedEventCallback;

		// clear expected event
		_expectedEventCallback = nullptr;
		(this->*eventCallback)(event);
	}
}
