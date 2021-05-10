/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <localisation/cs_AssetFilterStore.h>

class AssetFiltering : EventListener {
public:
	cs_ret_code_t init();

private:
	AssetFilterStore* _filterStore;

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
	 * metadata settings.
	 */
	bool filterInputResult(AssetFilter filter, const scanned_device_t& asset);

	// --------- Processing of accepted Assest ---------------

	/**
	 * Constructs the output of the filter for an accepted asset
	 * and dispatches it to specific handler.
	 *
	 * filter: the filter that accepted this device.
	 * device: the device that was accepted
	 */
	void processAcceptedAsset(AssetFilter filter, const scanned_device_t& asset);

	/**
	 * Handles further processing of accepted asset when
	 * filter.metadata.outputType.outputFormat is ShortAssetId
	 */
	void processAcceptedAsset(AssetFilter f, const scanned_device_t& asset, short_asset_id_t assetId);

	/**
	 * Handles further processing of accepted asset when
	 * filter.metadata.outputType.outputFormat is MAC
	 */
	void processAcceptedAsset(AssetFilter f, const scanned_device_t& asset, const mac_address_t& assetMac);


	// ---------- Util ---------------

	void logServiceData(scanned_device_t* scannedDevice);
public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


