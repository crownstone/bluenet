/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <ble/cs_BleCentral.h>
#include <ble/cs_CrownstoneCentral.h>
#include <dfu/cs_MeshDfuConstants.h>
#include <dfu/cs_MeshDfuTransport.h>
#include <events/cs_EventListener.h>
#include <common/cs_Component.h>
#include <util/cs_Coroutine.h>

/**
 * This bluenet component manages the progress and protocol of a crownstone-to-crownstone firmware update.
 * I.e. application layer dfu implementation.
 *
 * Usage:
 * To start an update, send a A CMD_MESH_DFU event with parameers:
 *  - target device mac address
 *  - operation (start, cancel, info).
 *
 * Immediate response (MESH_DFU_RESULT):
 *  - busy     already updating another stone
 *  - fail     starting did not succeed somehow, reverts state to idle.
 *  - wait for success  process will be started, subsequent calls will return busy until process terminates.
 *
 * Later (MESH_DFU_RESULT):
 *  In case wait for success was issued, another result will be sent.
 *  - ERR_SUCCESS: dfu process is done, verification checked
 *  - ERR_NO_CHANGE: an error occured in the process before any destructive operation in the target device
 *  - ERR_WRONG_STATE: an error occured in the process and target firmware is no longer in tact.
 *
 *  NOTE: corresponds roughly to dfu_Write_application.py
 */
class MeshDfuHost : public EventListener, public Component {
public:
	/**
	 * obtain pointers to ble components and listen();
	 * Resets _reconnectionAttemptsLeft.
	 */
	cs_ret_code_t init() override;

	/**
	 * return true if dfu process sucessfully started.
	 *
	 * Initializes this component if necessary.
	 */
	bool copyFirmwareTo(device_address_t target);

		// CheckDfu
#define XPHASES                                                                                            \
	X(Idle)                       /* nothing in progress */                                                \
	X(TargetTriggerDfuMode)       /* send dfu command and wait for disconnect */                           \
	X(WaitForTargetReboot)        /* wait until scans from target are received or timeout */               \
	X(ConnectTargetInDfuMode)     /* reconnect to target after scan or timeout */                          \
	X(DiscoverDfuCharacteristics) /* Check for DFU service and required characteristics */                 \
	X(TargetPreparing)            /* send PRN command */                                                   \
	X(TargetInitializing)         /* sending init packets */                                               \
	X(TargetUpdating)             /* sending firmware packets */                                           \
	X(TargetVerifying)            /**/                                                                     \
	X(Aborting)                   /* might leave the target in an ugly state but at least saves our ass */ \
	X(None)                       /* Boot up Phase, transfers to Idle when init has succeeded. */

	/**
	 * enum containing the phases list as values
	 */
	enum class Phase : uint8_t {
#define X(phase) phase,
		XPHASES
#undef X
	};

	/**
	 * phase to string method
	 */
	constexpr auto phaseName(Phase p) {
#define xstr(s) str(s)
#define str(s) #s
#define X(phase) case Phase::phase: { return xstr(phase); }
		switch(p){
			XPHASES
			default: { return "unknown"; }
		}
#undef X
#undef str
#undef xstr
	}

private:
	// ----------------------- runtime phase variables -----------------------

	/**
	 * state describing what the device is currently doing/waiting for
	 */
	Phase _phaseCurrent = Phase::None;
	/**
	 * upon phase complete, this phase will be checked for a subsequent action.
	 * (this can be user overridden to stop a dfu flood.)
	 */
	Phase _phaseOnComplete = Phase::None;

	/**
	 * Administration variables for the method setEventCallback(..);
	 *
	 * _expectedEvent is only valid if _onExpectedEvent is set.
	 * _onExpectedEvent is a pointer to member function taking an event_t&.
	 */
	CS_TYPE _expectedEvent;
	typedef void(MeshDfuHost::*ExpectedEventCallback)(event_t&);
	ExpectedEventCallback _onExpectedEvent = nullptr;

	/**
	 * Utility to call onEventCallbackTimeOut at desired moments.
	 */
	Coroutine _timeOutRoutine;
	typedef void(MeshDfuHost::*TimeoutCallback)();
	TimeoutCallback _onTimeout = nullptr;


	// ----------------------- ble related variables -----------------------

	/**
	 * cached ble component pointers
	 */
	CrownstoneCentral* _crownstoneCentral = nullptr;
	BleCentral* _bleCentral = nullptr;

	/**
	 * sub components
	 */
	MeshDfuTransport _meshDfuTransport;

	/**
	 * is listen() called?
	 */
	bool _listening = false;

	/**
	 * true if an attempt has already been made to put target in dfu mode during
	 * this of dfu-ing.
	 */
	bool _triedDfuCommand = false;

	/**
	 * current status of the connection. (Updated by several events.)
	 */
	bool _isCrownstoneCentralConnected = false;

	/**
	 * determines how many times a reconnect will be attempted until dfu is stopped.
	 * See MeshDfuConstants::DfuHostSettings::MaxReconnectionAttempts
	 */
	uint8_t _reconnectionAttemptsLeft = 0;

	/**
	 * the device to be dfu-d.
	 */
	device_address_t _targetDevice;



	// TODO: DEBUG
	device_address_t _debugTarget = {.address     = {0x35, 0x01, 0x59, 0x11, 0xE1,0xEE}, // 0xEE, 0xE1, 0x11, 0x59, 0x01, 0x35
									  .addressType = CS_ADDRESS_TYPE_RANDOM_STATIC};
	uint8_t ticks_until_start = 200;
	// END DEBUG


	// -------------------------------------------------------------------------------------
	// ---------------------------------- phase callbacks ----------------------------------
	// -------------------------------------------------------------------------------------

	/**
	 * clears any non-default values.
	 */
	bool startPhaseIdle();

	// ###### TargetTriggerDfuMode ######

	/**
	 * Connect through CrownstoneBle and wait for result. Retries if necessary.
	 *
	 * Continue with sendDfuCommand.
	 */
	bool startPhaseTargetTriggerDfuMode();

	/**
	 * Possibly retry connection if not _isCrownstoneCentralConnected yet.
	 *
	 * When connection is established, send dfu command and wait for disconnect.
	 * If max retries is reached, PhaseAborting will be started.
	 */
	void sendDfuCommand(event_t& event);

	/**
	 * checks if _isCrownstoneCentralConnected. If true:
	 * - wait for EVT_CS_CENTRAL_CONNECT_RESULT and abort if that times out.
	 *
	 * Else, completePhase.
	 */
	void verifyDisconnectAfterDfu(event_t& event);

	/**
	 * Next phase:
	 * - Phase::Abort if not successful, else
	 * - Phase::TargetPreparing
	 *
	 * Resets _reconnectionAttemptsLeft.
	 */
	Phase completePhaseTargetTriggerDfuMode();

	// ###### WaitForTargetReboot ######

	bool startWaitForTargetReboot();
	void checkScansForTarget(event_t& event);
	Phase completeWaitForTargetReboot();

	// ###### ConnectTargetInDfuMode ######

	/**
	 * Sets callbacks to wait for an incoming scan of the target or timeout
	 * in order to give ble central time to recover from the
	 * recent disconnect.
	 */
	bool startConnectTargetInDfuMode();

	/**
	 * Checks incoming scans to see if target is ready for connect
	 * and then executeConnectTargetInDfuMode.
	 */
	void connectTargetInDfuMode(event_t& event);

	/**
	 * call ble central connect and set event callback to wait for result.
	 */
	void executeConnectTargetInDfuMode();

	/**
	 * Check is BLE central is connected. If not, retry until max attempts is reached.
	 * If still not connected, abort().
	 *
	 * If connection established, continue to verifyDfuMode();
	 */
	void checkDfuTargetConnected();

	/**
	 * forwards to the previous method, discarding the event.
	 */
	void checkDfuTargetConnected(event_t& event);

	/**
	 * Next phase:
	 * - Phase::Abort if not successful, else
	 * - Phase::TargetPreparing
	 */
	Phase completeConnectTargetInDfuMode();

	// ###### DiscoverDfuCharacteristics ######

	/**
	 * Starts BLE central discovery.
	 * Sets a callback for BLE_CENTRAL_DISCOVERY_RESULT to goto onDiscoveryResult.
	 * Sets a timeout to complete the phase if it takes too long.
	 */
	bool startDiscoverDfuCharacteristics();

	/**
	 * Check result code, verifyDfuMode() and completePhase();
	 */
	void onDiscoveryResult(event_t& event);

	/**
	 * Checks if the required dfu characteristics are available on the connected
	 * device.
	 *
	 * Assumes BLE central is connected.
	 */
	bool verifyDfuMode();

	Phase completeDiscoverDfuCharacteristics();

	// ###### Aborting ######

	bool startPhaseAborting();
	void aborting(event_t& event);
	Phase completePhaseAborting();

	// ------------------------------------------------------------------------------------
	// ------------------------------- phase administration -------------------------------
	// ------------------------------------------------------------------------------------

	/**
	 * Calls startPhaseX where X is determined by the `phase` variable.
	 *
	 * Returns the return value of the called method.
	 * If returnvalue is true:
	 *  - _phaseCurrent is set to `phase`
	 *  - _phaseNext is set to None
	 *  - the start method was successful and has called waitForEvent.
	 * Else:
	 *  - nothing changed
	 */
	bool startPhase(Phase phase);

	/**
	 * Calls the startPhaseX function for the _phaseCurrent.
	 * (Used by startPhase to get into async 'processing mode'.)
	 */
	void startPhase(event_t& event);

	/**
	 * Call this method in the last phase event handler. It will:
	 *  - call completePhase(); and check intended next phase
	 *  - check the user set _phaseNext,
	 *  - call startPhase(nextPhase) depending on both these phase values.
	 *
	 * if phase status is reported by return value of completePhaseX methods.
	 * Return Phase::Aborting if unsuccesful to clean up as much as possible.
	 *
	 * completePhaseX methods must be synchronous.
	 */
	void completePhase();

	/**
	 * restarts the current phase.
	 */
	void restartPhase();

	/**
	 * starts the aborting phase.
	 */
	void abort();

	// ---------- callbacks ----------

	/**
	 * sets a callback to be called when the given event type is received.
	 * The event is passed into the callback for further inspection.
	 *
	 * Returns true if a previous callback was overriden by this call.
	 *
	 * Note: be sure to set the callback before the event is triggered.
	 * Note: event callback is cleared after it is received. If you want to repeat, set it again.
	 */
	bool setEventCallback(
			CS_TYPE evttype,
			ExpectedEventCallback callback);

	/**
	 * sets the timeout callback and delay before calling it.
	 */
	bool setTimeoutCallback(
			TimeoutCallback onTimeout,
			uint32_t timeoutMs = MeshDfuConstants::DfuHostSettings::DefaultTimeoutMs
			);

	/**
	 * sets the event callback to nullptr.
	 */
	void clearEventCallback();

	/**
	 * sets the timeout callback to nullptr.
	 */
	void clearTimeoutCallback();

	/**
	 * This is called by the timeoutRoutine to call the current _onTimeout callback.
	 */
	void onEventCallbackTimeOut();

	// ---------- phase start callbacks ----------

	bool startPhaseTargetPreparing();
	bool startPhaseTargetInitializing();
	bool startPhaseTargetUpdating();
	bool startPhaseTargetVerifying();

	// ---------- phase complete callbacks ----------

	/**
	 * completePhaseX methods finish administration for a phase and return
	 * what is the next intended phase. They must be synchronous.
	 *
	 * Called by completePhase.
	 */

	Phase completePhaseTargetPreparing();
	Phase completePhaseTargetInitializing();
	Phase completePhaseTargetUpdating();
	Phase completePhaseTargetVerifying();

	// -------------------------------------------------------------------------------------
	// --------------------------------------- utils ---------------------------------------
	// -------------------------------------------------------------------------------------

	/**
	 * check if ble component pointers are non-nullptr.
	 */
	bool isInitialized();

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
	bool isDfuProcessIdle();

	void connectToTarget();

	bool startDfu(device_address_t macaddr);

	/**
	 * sets all
	 */
	void reset();

public:
	/**
	 * Checks _expectedEvent and calls the callback if it matches and is non-null.
	 */
	virtual void handleEvent(event_t & event) override;
};
