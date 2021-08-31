/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>

#include <protocol/cs_Typedefs.h>
#include <protocol/mesh/cs_MeshModelPackets.h>

#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <localisation/cs_TrackableEvent.h>
#include <localisation/cs_AssetRecord.h>
#include <localisation/cs_AssetStore.h>
#include <localisation/cs_AssetHandler.h>

/**
 * This class implements the in-mesh computation of which crownstone
 * is nearest to a 'trackable' device.
 *
 * When a trackable device broadcasts an advertisement, and the TrackableParser
 * consequently dispatches a TrackableEvent, it is this class that performs the necessary
 * communication and computation to decide whether or not this crownstone is closest to
 * the trackable device.
 *
 * It is possible to enable this component on a subset of the crownstones in a mesh
 * if necessary (for optimization purposes perhaps). When doing that, the resulting nearest
 * crownstone will be an element of this subset.
 */
class NearestCrownstoneTracker : public EventListener, public Component {
	static constexpr uint8_t LAST_RECEIVED_TIMEOUT_THRESHOLD = 180;
	static constexpr uint16_t MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS = 100;

	static constexpr uint8_t RSSI_FALL_OFF_RATE_DB_PER_S = 1;
	static constexpr int8_t RSSI_CUT_OFF_THRESHOLD = -90;

	enum class FilterStrategy {
		TIME_OUT,
		RSSI_FALL_OFF
	};

	static constexpr auto FILTER_STRATEGY = FilterStrategy::TIME_OUT;

public:
	/**
	 * Caches CONFIG_CROWNSTONE_ID and AssetStore.
	 * If assetstore is not available, ERR_NOT_FOUND is returned.
	 */
	cs_ret_code_t init() override;

	/**
	 * returns desired minimal time to next advertisement.
	 */
	uint16_t handleAcceptedAsset(const scanned_device_t& asset, const short_asset_id_t& id);

private:
	// cached objects for efficiency
	stone_id_t _myStoneId;
	AssetStore* _assetStore;

	// -------------------------------------------
	// ------------- Incoming events -------------
	// -------------------------------------------

	/**
	 * Called on EVT_RECV_MESH_MSG events.
	 * Extract report from other crownstones that come in from the mesh
	 * and call onReceiveAssetReport.
	 */
	void handleMeshMsgEvent(event_t& evt);

	/**
	 * Called on EVT_ASSET_ACCEPTED events.
	 * Transforms an accepted asset event into a report and handles it
	 * by calling onReceivedAssetAdvertisement.
	 */
	void handleAssetAcceptedEvent(event_t& evt);

	/**
	 * Heart of the algorithm. See implementation for exact behaviour.
	 *
	 * This method implements what a crownstone needs to communicate when
	 * it directly receives an advertisement from a trackable device.
	 *
	 * Updates personal report, possibly updates winning report
	 * and possibly broadcasts a message to inform other devices
	 * in the mesh of relevant changes.
	 */
	void onReceiveAssetAdvertisement(report_asset_id_t& trackedEvent);

	/**
	 * Heart of the algorithm. See implementation for exact behaviour.
	 *
	 * This method implements what a crownstone needs to communicate when
	 * it receives a message from another crownstone containing information
	 * about a trackable device.
	 *
	 * Possibly updates winning report and possibly broadcasts
	 * a message to inform other devices in the mesh of relevant changes.
	 * E.g. when the updated winning report now loses from this devices
	 * personal report.
	 */
	void onReceiveAssetReport(report_asset_id_t& report, stone_id_t reporter);

	// -------------------------------------------
	// ------------- Outgoing events -------------
	// -------------------------------------------

	/**
	 * Sends a mesh broadcast for the given report.
	 * (stone id of reporter is contained in bluetooth metadata)
	 */
	void broadcastReport(report_asset_id_t& report);

	/**
	 * makes a report from the records personalRssi field
	 * and broadcast it over the mesh.
	 */
	void broadcastPersonalReport(asset_record_t& record);

	/**
	 * Sends a message over UART containing the winner, its rssi
	 * and the asset id.
	 */
	void sendUartUpdate(asset_record_t& record);

	/**
	 * Currently dispatches an event for CMD_SWITCH_[ON,OFF] depending on
	 * whether this crownstone is closest (ON) or not the closest.
	 *
	 * Also outputs the current winner to UART for demo purposes.
	 */
	void onWinnerChanged(bool winnerIsThisCrownstone);

	// ---------------------------------
	// ------------- Utils -------------
	// ---------------------------------

	/**
	 * saves the report in the _winning list.
	 */
	void saveWinningReport(asset_record_t& rec, compressed_rssi_data_t winningRssi, stone_id_t winningId);

	/**
	 * getRecord for assetId from assetStore and return it.
	 * return nullptr if rec->lastReceivedCounter is above threshold.
	 *
	 * `this` must be init()-ialized.
	 */
	asset_record_t* getRecordFiltered(const short_asset_id_t& assetId);

public:
	/**
	 * Handlers for:
	 * EVT_MESH_NEAREST_WITNESS_REPORT
	 * EVT_ASSET_ACCEPTED
	 * EVT_FILTERS_UPDATED
	 */
	void handleEvent(event_t &evt);
};
