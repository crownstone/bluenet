/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <dfu/cs_MeshDfuHost.h>
#include <logging/cs_Logger.h>

#define LogMeshDfuHostDebug LOGd

bool MeshDfuHost::copyTo(device_address_t target) {
	if (!ableToHostDfu()) {
		return false;
	}

	_targetDevice = target;

	startPhase(Phase::HostInitializing);
	return true;
}

// ------------------------------- phase administration -------------------------------

void MeshDfuHost::triggerTargetDfuMode() {
	_reconnectionAttemptsLeft = _reconnectionAttemptsDefault;

	_crownstoneCentral.connect(_targetDevice);
	waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, sendDfuCommand);

	LogMeshDfuHostDebug("waiting for cs central connect result");
}

void MeshDfuHost::sendDfuCommand(event_t& event) {
	if(!_isCrownstoneCentralConnected) {
		if(_reconnectionAttemptsLeft > 0) {
			LogMeshDfuHostDebug("connection failed, retrying");
			_reconnectionAttemptsLeft -= 1;

			_crownstoneCentral.connect(_targetDevice);
			waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, sendDfuCommand);

			LogMeshDfuHostDebug("waiting for cs central connect result");
		} else {
			stopDfu();
		}
	}

	_reconnectionAttemptsLeft = _reconnectionAttemptsDefault;
	_crownstoneCentral->write(CommandHandlerTypes::CTRL_CMD_GOTO_DFU, nullptr, 0);
	waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, reconnectAfterDfuCommand);

	LogMeshDfuHostDebug("waiting for disconnect after command goto dfu");
}

void MeshDfuHost::reconnectAfterDfuCommand(event_t& event) {
	if (_isCrownstoneCentralConnected) {
		LogMeshDfuHostDebug("incorrect state, device is still connected after dfu command.");
		waitForEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, reconnectAfterDfuCommand);
		LogMeshDfuHostDebug("waiting for disconnect after command goto dfu");
		return;
	}

	if(!_isBleCentralConnected) {
		if(_reconnectionAttemptsLeft > 0) {
			_reconnectionAttemptsLeft -= 1;

			_bleCentral.connect(_targetDevice);
			waitForEvent(CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT, reconnectAfterDfuCommand);
		} else {
			stopDfu();
		}
	}

	LogMeshDfuHostDebug("waiting for cs central connect result");
}

void MeshDfuHost::verifyDfuMode(event_t& event) {
	LogMeshDfuHostDebug("verify the dfu mode (TODO)");
}


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

//void MeshDfuHost::startPhase(Phase phase) {
//	_phaseCurrent = phase;
//
//	switch (phase) {
//		case Phase::Idle: {
//			break;
//		}
//		case Phase::HostInitializing: {
//			startPhaseHostInitializing();
//			break;
//		}
//		case Phase::TargetTriggerDfuMode: {
//			startPhaseTargetTriggerDfuMode();
//			break;
//		}
//		case Phase::TargetPreparing: {
//			startPhaseTargetPreparing();
//			break;
//		}
//		case Phase::TargetInitializing: {
//			startPhaseTargetInitializing();
//			break;
//		}
//		case Phase::TargetUpdating: {
//			startPhaseTargetUpdating();
//			break;
//		}
//		case Phase::TargetVerifying: {
//			startPhaseTargetVerifying();
//			break;
//		}
//		case Phase::Aborting: {
//			startPhaseAborting();
//			break;
//		}
//		case Phase::None: {
//			break;
//		}
//	}
//}
//
//void MeshDfuHost::restartPhase() {
//	startPhase(_phaseCurrent);
//}
//
//void MeshDfuHost::completePhaseAndGoto(Phase phaseNext) {
//	if (_phaseNextOverride != Phase::None) {
//		// LOG...
//		startPhase(_phaseNextOverride);
//	}
//
//	startPhase(phaseNext);
//}

bool MeshDfuHost::WaitForEvent(CS_TYPE evtToWaitOn, PhaseCallback callback) {
	if (_expectedEventCallback != nullptr) {
		return false;
	}

	_expectedEvent         = evtToWaitOn;
	_expectedEventCallback = callback;

	return true;
}

//bool MeshDfuHost::waitForReconnect() {
//
//}

// --------------------------------------- utils ---------------------------------------

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
		this->*eventCallback(event);
	}
}
