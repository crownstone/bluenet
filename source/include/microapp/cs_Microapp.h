/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 09, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_MicroappPackets.h>

/**
 * Class that enables the feature to run microapps on the firmware.
 *
 * This class:
 * - Handles control commands (from BLE and UART).
 * - Keeps up the state of microapps.
 * - Instantiates and uses classes MicroappStorage and MicroappProtocol.
 */
class Microapp : public EventListener {
public:
	static Microapp& getInstance() {
		static Microapp instance;
		return instance;
	}

	/**
	 * Initialize storage, and load microapps in normal mode.
	 * Checks operation mode and only actually initializes for some modes.
	 */
	void init(OperationMode operationMode);

private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	Microapp();
	Microapp(Microapp const&)       = delete;
	void operator=(Microapp const&) = delete;

	/**
	 * The state of each microapp.
	 */
	microapp_state_t _states[g_MICROAPP_COUNT];

	/**
	 * Local flag to indicate that ram section has been loaded.
	 */
	bool _loaded                  = false;

	/**
	 * Keep up which microapp is currently being operated on.
	 * Used for factory reset.
	 * Set to -1 when not operating on anything.
	 */
	int16_t _currentMicroappIndex = -1;

	/**
	 * Whether we are in factory reset mode.
	 */
	bool _factoryResetMode        = false;

	void loadApps();

	void loadState(uint8_t index);

	/**
	 * Update state from operating data in IPC.
	 *
	 * For example, when a reboot has happened when a microapp was running, this updates the state accordingly.
	 */
	void updateStateFromOperatingData(uint8_t index);

	/**
	 * Validates app: compare checksum.
	 * App state is updated in this call, make sure to store the state afterwards.
	 */
	cs_ret_code_t validateApp(uint8_t index);

	/**
	 * Enables app: checks sdk version.
	 * App state is updated in this call, make sure to store the state afterwards.
	 */
	cs_ret_code_t enableApp(uint8_t index);

	/**
	 * Start app, if it passed all tests according to cached state.
	 */
	cs_ret_code_t startApp(uint8_t index);

	/**
	 * Resets app state in ram only.
	 */
	void resetState(uint8_t index);

	/**
	 * Resets app state tests in ram only.
	 */
	void resetTestState(uint8_t index);

	/**
	 * Store app state to flash.
	 */
	cs_ret_code_t storeState(uint8_t index);

	/**
	 * Checks app state and returns true if this app is allowed to run.
	 */
	bool canRunApp(uint8_t index);

	/**
	 * To be called every tick.
	 * Calls all microapps.
	 */
	void tick();

	/**
	 * Handle control commands.
	 */
	cs_ret_code_t handleGetInfo(cs_result_t& result);
	cs_ret_code_t handleUpload(microapp_upload_internal_t* packet);
	cs_ret_code_t handleValidate(microapp_ctrl_header_t* packet);
	cs_ret_code_t handleRemove(microapp_ctrl_header_t* packet);
	cs_ret_code_t handleEnable(microapp_ctrl_header_t* packet);
	cs_ret_code_t handleDisable(microapp_ctrl_header_t* packet);

	/**
	 * Checks if control command header is ok.
	 */
	cs_ret_code_t checkHeader(microapp_ctrl_header_t* packet);

	/**
	 * Removes all microapps and sends an event when done: EVT_MICROAPP_FACTORY_RESET_DONE.
	 */
	cs_ret_code_t factoryReset();

	/**
	 * Remove next microapp and sends an event when done: EVT_MICROAPP_FACTORY_RESET_DONE.
	 */
	cs_ret_code_t resumeFactoryReset();

	/**
	 * Handle microapp storage event.
	 *
	 * For now, this just handles CMD_RESOLVE_ASYNC_CONTROL_COMMAND.
	 * We should register an event handler instead.
	 */
	void onStorageEvent(cs_async_result_t& event);

	/**
	 * Handle incoming events.
	 */
	void handleEvent(event_t& event);
};
