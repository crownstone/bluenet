/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <localisation/cs_AssetFilterStore.h>
#include <localisation/cs_AssetFilterSyncer.h>
#include <localisation/cs_AssetHandler.h>
#include <localisation/cs_AssetForwarder.h>

class AssetFiltering : EventListener {
public:
	cs_ret_code_t init();

	// TODO: check if master version != 0 before using filters.

	void setAssetHandlerMac(AssetHandlerMac* assetHandlerMac);
	void setAssetHandlerShortId(AssetHandlerShortId* assetHandlerMac);

private:
	AssetFilterStore* _filterStore   = nullptr;
	AssetFilterSyncer* _filterSyncer = nullptr;
	AssetForwarder* _assetForwarder   = nullptr;

	/**
	 * This handleAcceptedAsset callback will be called for each filter
	 * that has output type mac and have accepted the incoming asset.
	 */
	AssetHandlerMac* _assetHandlerMac = nullptr;

	/**
	 * This handleAcceptedAsset callback will be called for each filter
	 * that has output type ShortAssetId and have accepted the incoming asset.
	 */
	AssetHandlerShortId* _assetHandlerShortId = nullptr;

	/**
	 * Dispatches a TrackedEvent for the given advertisement.
	 */
	void handleScannedDevice(const scanned_device_t& asset);

	/**
	 * Handles a specific filter.
	 */
	void processFilter(AssetFilter f, const scanned_device_t& asset);

	/**
	 * Returns true if the device passes the filter according to its
	 * metadata settings. Returns false otherwise.
	 */
	bool filterInputResult(AssetFilter filter, const scanned_device_t& asset);

	/**
	 * Returns a short_asset_id_t based on the configured selection of data
	 * in metadata.outputType.inFormat. If the data is not sufficient, a default
	 * constructed object is returned. (Data not sufficient can be detected:
	 * filterInputResult will return false in that case.)
	 */
	short_asset_id_t filterOutputResultShortAssetId(AssetFilter filter, const scanned_device_t& asset);

	// --------- Processing of accepted Assest ---------------

	/**
	 * Constructs the output of the filter for an accepted asset
	 * and dispatches it to one of the specific handlers, among which:
	 * - processAcceptedAssetMac,
	 * - _assetHandlerMac
	 * - _assetHandlerShortId
	 *
	 * filter: the filter that accepted this device.
	 * device: the device that was accepted
	 */
	void processAcceptedAsset(AssetFilter filter, const scanned_device_t& asset);

	void dispatchAcceptedAssetMacToMesh(AssetFilter filter, const scanned_device_t& asset);


public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


