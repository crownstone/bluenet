/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <dfu/cs_MeshDfuHost.h>

bool MeshDfuHost::copyTo(device_address_t target) {
	if (!ableToHostDfu()) {
		return false;
	}

	_targetDevice = target;

	startPhase(Phase::HostInitializing);
	return true;
}

// ------------------------------- phase administration -------------------------------

// Phase::HostInitializing

void MeshDfuHost::startPhaseHostInitializing() {

	// write target to flash
	cs_state_data_t targetDeviceStateData (CS_TYPE::DFU_TARGET_DEVICE, _targetDevice, sizeof(_targetDevice);
	State::getInstance().set(targetDeviceStateData, PersistenceMode::FLASH);

	waitForEvent(CS_TYPE::EVT_FLASH_WRITE_DONE, finalizeHostInitializing);
}

void MeshDfuHost::finalizeHostInitializing(event_t& event) {
	// check target?

	completePhaseAndGoto(Phase::TargetTriggerDfuMode);
}

// Phase::TargetTriggerDfuMode

void MeshDfuHost::startPhaseTargetTriggerDfuMode() {

	// send command goto dfu
	// wait for disconnect
	// reconnect wait for connect success
}

void MeshDfuHost::finalizeTargetTriggerDfuMode() {
	// check dfu mode success.
	completePhaseAndGoto(Phase::TargetPreparing);
}

// Phase::TargetPreparing

void MeshDfuHost::startPhaseTargetPreparing() {

	// __send_prn();
}

void MeshDfuHost::finalizeTargetPreparing() {
	// inspect result
	completePhaseAndGoto(Phase::TargetInitializing);
}

void MeshDfuHost::startPhaseTargetInitializing() {}
void MeshDfuHost::startPhaseTargetUpdating() {}
void MeshDfuHost::startPhaseTargetVerifying() {}

void MeshDfuHost::startPhaseAborting() {
	// invalidate _targetDevice (in flash)
}

// ---------------------------- progress related callbacks ----------------------------

void MeshDfuHost::startPhase(Phase phase) {
	_phaseCurrent = phase;

	switch (phase) {
		case Phase::Idle: {
			break;
		}
		case Phase::HostInitializing: {
			startPhaseHostInitializing();
			break;
		}
		case Phase::TargetTriggerDfuMode: {
			startPhaseTargetTriggerDfuMode();
			break;
		}
		case Phase::TargetPreparing: {
			startPhaseTargetPreparing();
			break;
		}
		case Phase::TargetInitializing: {
			startPhaseTargetInitializing();
			break;
		}
		case Phase::TargetUpdating: {
			startPhaseTargetUpdating();
			break;
		}
		case Phase::TargetVerifying: {
			startPhaseTargetVerifying();
			break;
		}
		case Phase::Aborting: {
			startPhaseAborting();
			break;
		}
		case Phase::None: {
			break;
		}
	}
}

void MeshDfuHost::completePhaseAndGoto(Phase phaseNext) {
	if (_phaseNextOverride != Phase::None) {
		// LOG...
		startPhase(_phaseNextOverride);
	}

	startPhase(phaseNext);
}

bool MeshDfuHost::WaitForEvent(CS_TYPE evtToWaitOn, PhaseCallback callback) {
	if (_expectedEventCallback != nullptr) {
		return false;
	}

	_expectedEvent         = evtToWaitOn;
	_expectedEventCallback = callback;

	return true;
}

// --------------------------------------- utils ---------------------------------------

bool MeshDfuHost::ableToLaunchDfu() {
	return haveInitPacket() && dfuProcessIdle();
}

bool MeshDfuHost::dfuProcessIdle() {
	return _phaseCurrent == Phase::Idle;
}

void MeshDfuHost::handleEvent(event_t& event) {
	if (_expectedEventCallback != nullptr && event.type == _expectedEvent) {
		// create local copy
		auto eventCallback = _expectedEventCallback;

		// clear expected event
		_expectedEventCallback = nullptr;
		eventCallback(event);
	}
}
