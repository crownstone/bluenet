/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 12 Apr., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "events/cs_EventListener.h"
#include "common/cs_Types.h"
#include "mesh/cs_MeshModel.h"
#include <mesh/cs_MeshAdvertiser.h>

extern "C" {
#include <nrf_mesh_config_app.h>
#include <nrf_mesh_defines.h>
#include <device_state_manager.h>
}

class Mesh : EventListener {
public:
	/**
	 * Get a reference to the Mesh object.
	 */
	static Mesh& getInstance();

	/**
	 * Init the mesh.
	 */
	void init(const boards_config_t& board);

	/**
	 * Start the mesh.
	 *
	 * Start using the radio and handle incoming messages.
	 */
	void start();

	/**
	 * Stop the mesh.
	 *
	 * Stops all radio usage.
	 */
	void stop();

	/**
	 * Factory reset.
	 *
	 * Clear all stored data.
	 * Will send event EVT_MESH_FACTORY_RESET when done.
	 */
	void factoryReset();

	/**
	 * Advertise as iBeacon.
	 */
	void advertise(IBeacon* ibeacon);

	/**
	 * Internal usage
	 */
	static void staticModelsInitCallback() {
		Mesh::getInstance().modelsInitCallback();
	}
	void modelsInitCallback();

	void handleEvent(event_t & event);

	void factoryResetDone();

private:
	//! Constructor, singleton, thus made private
	Mesh();

	//! Copy constructor, singleton, thus made private
	Mesh(Mesh const&);

	//! Assignment operator, singleton, thus made private
	void operator=(Mesh const &);

	void provisionSelf(uint16_t id);
	void provisionLoad();

	bool _isProvisioned = false;
	/** Address of this node */
	uint16_t _ownAddress;

	uint8_t _netkey[NRF_MESH_KEY_SIZE];
	dsm_handle_t _netkeyHandle = DSM_HANDLE_INVALID;
	uint8_t _appkey[NRF_MESH_KEY_SIZE];
	dsm_handle_t _appkeyHandle = DSM_HANDLE_INVALID;
	uint8_t _devkey[NRF_MESH_KEY_SIZE];
	dsm_handle_t _devkeyHandle = DSM_HANDLE_INVALID;
	dsm_handle_t _groupAddressHandle = DSM_HANDLE_INVALID;

	MeshAdvertiser _advertiser;
	MeshModel _model;
	uint32_t _sendStateTimeCountdown = MESH_SEND_TIME_INTERVAL_MS / TICK_INTERVAL_MS;
	bool _performingFactoryReset = false;
};
