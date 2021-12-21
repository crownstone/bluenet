/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include<events/cs_EventListener.h>


/**
 * This bluenet component manages the progress and protocol of a crownstone-to-crownstone firmware update.
 *
 * Usage:
 * To start an update, send a A CMD_MESH_DFU event with parameers:
 *  - target device mac address
 *  - operation (start, cancel, info).
 * Response:
 *  - busy     already updating another stone
 *  - fail     starting did not succeed somehow, reverts state to idle.
 *  - success  process will be started, subsequent calls will return busy until process terminates.
 */
class MeshDfuHost : public EventListener {
private:
	enum class Phase {
		Idle = 0,
		HostInitializing,     // writing target settings to flash
		TargetTriggerDfuMode, // sending dfu command and waiting for reset
		TargetInitializing,   // sending init packets
		TargetUpdating,       // sending firmware packets
		TargetVerifying,
		Aborting,            // might leave the target in an ugly state but at least saves our ass.
		None = 0xFF
	};

	// basically for any operation we first have to do the connection dance.
//		ConnectingToTarget, // this phase might need a different place because it is passed multiple times during dfu

	/**
	 * state describing what the device is currently doing/waiting for
	 */
	Phase _phaseCurrent= Phase::Idle;
	/**
	 * upon phase complete, this phase will be checked for a subsequent action.
	 */
	Phase _phaseNext = Phase::None;

	// ------------------------------------------------------------------------------------
	// ---------------------------- progress related callbacks ----------------------------
	// ------------------------------------------------------------------------------------

	/**
	 * Checks if there is a _phaseNext that is not None. If so, execute/prepare transition to that state.
	 * Otherwise, execute/prepare the standard dfu flow.
	 */
	void onPhaseComplete();

	/**
	 * Clears all fields and data related to the Mesh DFU progress.
	 */
	void resetProgress();

	/**
	 * Sets progress from Started to Initialized or resetProgress() on failure.
	 */
	void onFlashWriteComplete();

	/**
	 * Checks if any progress was persisted and try to recover.
	 */
	void onBoot();

	bool storeDfuTarget(device_address_t macaddr);
	bool startDfu(device_address_t macaddr);

	// -------------------------------------------------------------------------------------
	// --------------------------------------- utils ---------------------------------------
	// -------------------------------------------------------------------------------------

	/**
	 * returns true if this device has an init packet and no already running dfu process.
	 */
	bool ableToHostDfu();

	bool haveInitPacket();
	bool dfuProcessIdle();

public:
	virtual void handleEvent(event_t & event) override;
};
