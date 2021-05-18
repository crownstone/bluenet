/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 12 Apr., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <mesh/cs_MeshAdvertiser.h>
#include <mesh/cs_MeshCore.h>
#include <mesh/cs_MeshModelMulticast.h>
#include <mesh/cs_MeshModelMulticastAcked.h>
#include <mesh/cs_MeshModelMulticastNeighbours.h>
#include <mesh/cs_MeshModelUnicast.h>
#include <mesh/cs_MeshModelSelector.h>
#include <mesh/cs_MeshMsgHandler.h>
#include <mesh/cs_MeshMsgSender.h>
#include <mesh/cs_MeshScanner.h>

/**
 * Class that manages all mesh classes:
 * - Core
 * - Models
 * - Message handler
 * - Message sender
 * - Scanner
 * - Advertiser
 * Also:
 * - Starts and retries sync requests.
 * - Sends crownstone state at a regular interval.
 */
class Mesh : public EventListener {
public:
	/**
	 * Get a reference to the Mesh object.
	 */
	static Mesh& getInstance();

	/**
	 * Init the mesh.
	 *
	 * @return ERR_SUCCESS             Initialized successfully.
	 * @return ERR_WRONG_STATE         Flash pages should be erased.
	 */
	cs_ret_code_t init(const boards_config_t& board);

	/**
	 * Checks if flash pages have valid data.
	 *
	 * If not, the pages will be erased, wait for event EVT_MESH_PAGES_ERASED.
	 * Has to be done before storage is initialized.
	 *
	 * @return true when valid.
	 */
	bool checkFlashValid();

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
	 * Init the advertiser.
	 */
	void initAdvertiser();

	/**
	 * Start advertising as iBeacon.
	 */
	void advertiseIbeacon();

	/**
	 * Stop advertising.
	 */
	void stopAdvertising();

	/**
	 * Start synchronization of data with other nodes in mesh.
	 */
	void startSync();

	/** Internal usage */
	void handleEvent(event_t & event);

private:
	//! Constructor, singleton, thus made private
	Mesh();

	//! Copy constructor, singleton, thus made private
	Mesh(Mesh const&) = delete;

	//! Assignment operator, singleton, thus made private
	Mesh& operator=(Mesh const &) = delete;

	MeshCore*                     _core;
	MeshModelMulticast            _modelMulticast;
	MeshModelMulticastNeighbours  _modelMulticastNeighbours;
	MeshModelMulticastAcked       _modelMulticastAcked;
	MeshModelUnicast              _modelUnicast;
	MeshModelSelector             _modelSelector;
	MeshMsgHandler                _msgHandler;
	MeshMsgSender                 _msgSender;
	MeshAdvertiser                _advertiser;
	MeshScanner                   _scanner;

	BOOL _enabled = true;

	// Sync request
	bool _synced = false;
	uint32_t _syncCountdown = -1;
	uint32_t _syncFailedCountdown = 0;

	/**
	 * Dispatches an internal event to request what data this crownstone needs to receive
	 * from the mesh. Afterwards, broadcasts a BT message in order to obtain the desired information.
	 *
	 * Assumes all event handlers that are interested in obtaining data are registered
	 * with the event dispatcher.
	 *
	 * @param [propagateSyncMessageOverMesh] if set to false no mesh messages will be sent. (Use this internally to check if device is synced.)
	 * 
	 * @return true  When request was necessary. If propagateSyncMessageOverMesh is true, a mesh message will be sent to resolve the sync.
	 * @return false When nothing had to be requested, so everything is synced.
	 */
	bool requestSync(bool propagateSyncMessageOverMesh = true);

	void initModels();

	void configureModels(dsm_handle_t appkeyHandle);

	void onTick(uint32_t tickCount);
};
