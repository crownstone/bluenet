/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>
#include <localisation/cs_AssetFilterStore.h>
#include <localisation/cs_AssetFilterSyncer.h>
#include <localisation/cs_AssetForwarder.h>
#include <localisation/cs_AssetHandler.h>
#include <localisation/cs_NearestCrownstoneTracker.h>
#include <util/cs_Utils.h>

class AssetFiltering : public EventListener, public Component {
public:
	/**
	 * Initialize this class.
	 *
	 * Constructs and initializes member classes.
	 * Checks result of previous call to init.
	 */
	cs_ret_code_t init();

	/**
	 * Returns true if init has been called successfully.
	 */
	bool isInitialized();

private:
	AssetFilterStore* _filterStore   = nullptr;
	AssetFilterSyncer* _filterSyncer = nullptr;
	AssetForwarder* _assetForwarder   = nullptr;
	AssetStore* _assetStore = nullptr;

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	NearestCrownstoneTracker* _nearestCrownstoneTracker = nullptr;
#endif

	// Keeps up the init state of this class.
	enum class AssetFilteringState {
		NONE,           // Nothing happened yet.
		INIT_FAILED,    // Init failed.
		INIT_SUCCESS    // Init was successful.
	};
	AssetFilteringState _initState = AssetFilteringState::NONE;

	/**
	 * Initializes this class.
	 *
	 * Constructs and initializes member classes.
	 */
	cs_ret_code_t initInternal();

	/**
	 * bitmask of filters per type to describe the acceptance of an asset.
	 */
	struct filter_output_bitmasks_t {
		uint8_t _forwardAssetId;
		uint8_t _forwardMac;
		uint8_t _nearestAssetId;

		uint8_t combined() const { return _forwardMac | _nearestAssetId | _forwardAssetId; }
		uint8_t assetId() const { return _forwardAssetId | _nearestAssetId; }

		/**
		 * returns the lowest filter id that accepted the asset.
		 *
		 * This filter is used to generate the short asset id when
		 * no primarySidFilter is available.
		 */
		uint8_t const primaryFilter() const { return BLEutil::lowestBitSet(combined()); }

		/**
		 * returns the lowest filter id that accepted the asset and was
		 * configured specifically to output a short asset id.
		 * E.g. AssetFilterOutputFormat::ShortAssetIdOverMesh.
		 *
		 * This filter is used to generate the short asset id when available.
		 */
		uint8_t primaryAssetIdFilter() const { return BLEutil::lowestBitSet(assetId()); }
	};


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
	 * Loop over inclusion filters and check which filters are accepting.
	 * Return a set of bitmasks containing the result.
	 */
	filter_output_bitmasks_t getAcceptedBitmasks(const scanned_device_t& device);

	/**
	 * Returns the filter object that is to be used to generate the asset id.
	 * This will be the primary sid filter, when it exists, and the primary filter
	 * otherwise.
	 *
	 * Returns an AssetFilter with nullptr as _data when none of the masks bits are set.
	 */
	AssetFilter filterToUseForShortAssetId(const filter_output_bitmasks_t& masks);

	void handleScannedDevice(filter_output_bitmasks_t masks, const scanned_device_t& asset);

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

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


