/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <third/std/function.h>

extern "C" {
#include <nrf_mesh_config_app.h>
#include <nrf_mesh_defines.h>
#include <device_state_manager.h>
}

/**
 * Class that:
 * - Initializes the mesh.
 * - Handles mesh events.
 * - Performs provisioning.
 * - Manages mesh flash.
 */
class MeshCore: EventListener {
public:
	/**
	 * Get a reference to the MeshCore object.
	 */
	static MeshCore& getInstance();

	/** Callback function definition. */
	typedef function<void(dsm_handle_t appkeyHandle)> callback_model_init_t;

	/** Callback function definition. */
	typedef function<void(const nrf_mesh_adv_packet_rx_data_t *scanData)> callback_scan_t;

	/**
	 * Register a callback function that's called when the models should be initialized.
	 *
	 * Must be done before init.
	 */
	void registerModelInitCallback(const callback_model_init_t& closure);

	/**
	 * Register a callback function that's called when a device was scanned.
	 *
	 * Must be done before init.
	 */
	void registerScanCallback(const callback_scan_t& closure);

	/**
	 * Whether flash pages have valid data.
	 *
	 * If not, the pages should be erased.
	 */
	bool isFlashValid();

	/**
	 * Init the mesh.
	 *
	 * Register callbacks first.
	 *
	 * @return ERR_SUCCESS             Initialized successfully.
	 * @return ERR_WRONG_STATE         Flash pages should be erased.
	 */
	cs_ret_code_t init(const boards_config_t& board);

	void start();

	/**
	 * Factory reset.
	 *
	 * Clear all stored data.
	 * Will send event EVT_MESH_FACTORY_RESET when done.
	 */
	void factoryReset();

	/**
	 * Erase all flash pages used by the mesh for storage.
	 */
	cs_ret_code_t eraseAllPages();

	/**
	 * Internal usage
	 */
	void factoryResetDone();

	/** Internal usage */
	void handleEvent(event_t & event);

	/** Internal usage */
	void modelsInitCallback();

	/** Internal usage */
	void scanCallback(const nrf_mesh_adv_packet_rx_data_t *scanData);

private:
	//! Constructor, singleton, thus made private
	MeshCore();

	//! Copy constructor, singleton, thus made private
	MeshCore(MeshCore const&) = delete;

	//! Assignment operator, singleton, thus made private
	MeshCore& operator=(MeshCore const &) = delete;

	void provisionSelf(uint16_t id);
	void provisionLoad();

	bool _isProvisioned = false;

	/** Address of this node */
	uint16_t _ownAddress = 0;

	callback_scan_t _scanCallback = nullptr;

	callback_model_init_t _modelInitCallback = nullptr;

	uint8_t _netkey[NRF_MESH_KEY_SIZE];
	dsm_handle_t _netkeyHandle = DSM_HANDLE_INVALID;
	uint8_t _appkey[NRF_MESH_KEY_SIZE];
	dsm_handle_t _appkeyHandle = DSM_HANDLE_INVALID;
	uint8_t _devkey[NRF_MESH_KEY_SIZE];
	dsm_handle_t _devkeyHandle = DSM_HANDLE_INVALID;

	// Flash
	bool _performingFactoryReset = false;

	void getFlashPages(void* & startAddress, void* & endAddress);
};
