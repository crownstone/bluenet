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

#include <util/cs_BitUtils.h>

class AssetFiltering : EventListener {
public:
	cs_ret_code_t init();

private:
	AssetFilterStore* _filterStore   = nullptr;
	AssetFilterSyncer* _filterSyncer = nullptr;
	AssetForwarder* _assetForwarder   = nullptr;

	/**
	 * Returns true if init has been called successfully and
	 * _filterStore, _filterSyncer and _assetForwarder are non-nullptr.
	 */
	bool isInitialized();

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
	bool filterAcceptsScannedDevice(AssetFilter filter, const scanned_device_t& asset);

	/**
	 * Returns a short_asset_id_t based on the configured selection of data
	 * in metadata.outputType.inFormat. If the data is not sufficient, a default
	 * constructed object is returned. (Data not sufficient can be detected:
	 * filterInputResult will return false in that case.)
	 */
	short_asset_id_t filterOutputResultShortAssetId(AssetFilter filter, const scanned_device_t& asset);

	// --------- Processing of accepted Assest ---------------

	/**
	 * bitmask of filters per type.
	 */
	struct filterBitmasks {
		uint8_t _forwardSid;
		uint8_t _forwardMac;
		uint8_t _nearestSid;

		uint8_t combined() { return _forwardMac | _nearestSid | _forwardSid; }
		uint8_t sid() { return _forwardSid | _nearestSid; }

		uint8_t primaryFilter() { return lowestBitSet(combined()); }
		uint8_t primarySidFilter() { return lowestBitSet(sid()); }
	};

	/**
	 * Loop over inclusion filters and check which filters are accepting.
	 * Return a set of bitmasks containing the result.
	 */
	filterBitmasks getAcceptedBitmasks(const scanned_device_t& device);

	/**
	 * Returns true if there is a filter that rejects this device.
	 * (Does not check if the filterstore is ready.)
	 */
	bool isAssetRejected(const scanned_device_t& device);

	/**
	 * Constructs the output of the filter for an accepted asset
	 * and dispatches it to one of the specific handlers, among which:
	 * - processAcceptedAssetMac,
	 * - _assetHandlerMac
	 * - _assetHandlerShortId
	 *
	 * filter: the filter that accepted this device.
	 * device: the device that was accepted
	 *
	 * Returns the filters outputType.outputFormat.
	 */
	AssetFilterOutputFormat processAcceptedAsset(AssetFilter filter, const scanned_device_t& asset);

	void dispatchAcceptedAssetEvent(AssetFilter filter, const scanned_device_t& asset);


public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


