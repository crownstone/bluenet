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
class Microapp: public EventListener {
public:
	static Microapp& getInstance() {
		static Microapp instance;
		return instance;
	}

	/**
	 * Initialize storage, and load microapps.
	 */
	void init();

private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	Microapp();
	Microapp(Microapp const&) = delete;
	void operator=(Microapp const &) = delete;

	/**
	 * The state of each microapp.
	 */
	microapp_state_t _states[MAX_MICROAPPS];

	/**
	 * Local flag to indicate that ram section has been loaded.
	 */
	bool _loaded = false;

	void loadApps();

	void loadState(uint8_t index);

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
	 * Handle incoming events.
	 */
	void handleEvent(event_t & event);
};
