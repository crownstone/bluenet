/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <events/cs_EventListener.h>
#include <optional>

/**
 * This bluenet component manages the progress and protocol of a crownstone-to-crownstone firmware update.
 * I.e. application layer dfu implementation.
 *
 * Usage:
 * To start an update, send a A CMD_MESH_DFU event with parameers:
 *  - target device mac address
 *  - operation (start, cancel, info).
 * Response:
 *  - busy     already updating another stone
 *  - fail     starting did not succeed somehow, reverts state to idle.
 *  - success  process will be started, subsequent calls will return busy until process terminates.
 *
 *  NOTE: corresponds roughly to dfu_Write_application.py
 */
class MeshDfuHost : public EventListener {
	/**
	 * return true if dfu process sucessfully started.
	 */
	bool copyTo(device_address_t target);

private:
	enum class Phase {
		Idle = 0,
		HostInitializing,     // writing target settings to flash
		TargetTriggerDfuMode, // sending dfu command and waiting for reset
		TargetPreparing,      // send PRN command
		TargetInitializing,   // sending init packets
		TargetUpdating,       // sending firmware packets
		TargetVerifying,
		Aborting,            // might leave the target in an ugly state but at least saves our ass.
		Booting,			// during boot up, storage isn't available yet so we don't know if we need to continue a previous dfu progress.
		None = 0xFF
	};

	/**
	 * state describing what the device is currently doing/waiting for
	 */
	Phase _phaseCurrent= Phase::None;
	/**
	 * upon phase complete, this phase will be checked for a subsequent action.
	 * (this can be user overridden to stop a dfu flood.)
	 */
	Phase _phaseNextOverride = Phase::None;

	/**
	 * the device to be dfu-d. must be persisted during HostInitialization
	 */
	device_address_t _targetDevice;


	/**
	 * Administration variables for the method waiteForEvent(..);
	 *
	 * _expectedEvent is only valid if a callback is set.
	 */
	CS_TYPE _expectedEvent;
	typedef void(*PhaseCallback)(event_t&);
	PhaseCallback _expectedEventCallback = nullptr;


	// -------------------------------------------------------------------------------------
	// ---------------------------------- phase callbacks ----------------------------------
	// -------------------------------------------------------------------------------------

	/**
	 *  start functions
	 *
	 *  only intended to be called by startPhase(Phase phase)
	 *  these don't set the _phaseCurrent value.
	 */

	void startPhaseIdle();
	void startPhaseHostInitializing();
	void startPhaseTargetTriggerDfuMode();
	void startPhaseTargetPreparing();
	void startPhaseTargetInitializing();
	void startPhaseTargetUpdating();
	void startPhaseTargetVerifying();
	void startPhaseAborting();
	void startPhaseBooting();

	/**
	 * finalize methods
	 *
	 * TODO: rename these to what they actually do?
	 */

	void finalizePhaseHostInitializing(event_t& event);
	void finalizePhaseTargetTriggerDfuMode(event_t& event);
	void finalizePhaseTargetPreparing(event_t& event);
	void finalizePhaseTargetInitializing(event_t& event);
	void finalizePhaseTargetUpdating(event_t& event);
	void finalizePhaseTargetVerifying(event_t& event);
	void finalizePhaseAborting(event_t& event);
	void finalizePhaseBooting(event_t& event);

	// ------------------------------------------------------------------------------------
	// ------------------------------- phase administration -------------------------------
	// ------------------------------------------------------------------------------------

	/**
	 * starts given phase and set _phaseCurrent (without additional checks).
	 */
	void startPhase(Phase phase);

	/**
	 * sets a callback to be called when the given event type is received.
	 * The event is passed into the callback for further inspection.
	 *
	 * Returns true if the callback was succesfully set.
	 * Returns false if another event is already awaited.
	 */
	bool WaitForEvent(CS_TYPE evttype, PhaseCallback callback);

	/**
	 * This method is called on successful finalizePhaseXYZ();
	 * The passed parameter is the intended next phase of the finalize method,
	 * which may be overriden by a user set phase (likely abort).
	 *
	 * _phaseNextOverride has priority over phaseNext.
	 *
	 */
	void completePhaseAndGoto(Phase phaseNext);

	// ------------------------------------------------------------------------------------
	// ---------------------------- progress related callbacks ----------------------------
	// ------------------------------------------------------------------------------------

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


//	// -----------------------------------------------------------------------------------
//	// --------------------------------- event callbacks ---------------------------------
//	// -----------------------------------------------------------------------------------
//
//	void onTimeout();
//	void onConnect(cs_ret_code_t retCode);
//	void onDisconnect();
//	void onDiscovery(ble_central_discovery_t& result);
//	void onDiscoveryDone(cs_ret_code_t retCode);
//	void onRead(ble_central_read_result_t& result);
//	void onReadDuringConnect(ble_central_read_result_t& result);
//	void onWrite(cs_ret_code_t result);
//	void onNotification(ble_central_notification_t& result);

	// -------------------------------------------------------------------------------------
	// --------------------------------------- utils ---------------------------------------
	// -------------------------------------------------------------------------------------

	/**
	 * returns true if this device has an init packet and no already running dfu process.
	 */
	bool ableToLaunchDfu();

	/**
	 * Is the dfu packet written to ICP flash page?
	 */
	bool haveInitPacket();

	/**
	 * No current dfu operations running or planned?
	 */
	bool dfuProcessIdle();

public:
	/**
	 * Checks _expectedEvent and calls the callback if it matches and is non-null.
	 */
	virtual void handleEvent(event_t & event) override;
};
