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

protected:
	/**
	 * returns the following child-components in a vector of pointers:
	 *   _filterStore,
	 *   _filterSyncer,
	 *   _assetForwarder,
	 *   _assetStore
	 *   _nearestCrownstoneTracker (if included in build)
	 */
	virtual std::vector<Component*> getChildren() override;

private:
	AssetFilterStore* _filterStore   = nullptr;
	AssetFilterSyncer* _filterSyncer = nullptr;
	AssetForwarder* _assetForwarder  = nullptr;
	AssetStore* _assetStore          = nullptr;

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	NearestCrownstoneTracker* _nearestCrownstoneTracker = nullptr;
#endif

	// Keeps up the init state of this class.
	enum class AssetFilteringState {
		NONE,         // Nothing happened yet.
		INIT_FAILED,  // Init failed.
		INIT_SUCCESS  // Init was successful.
	};
	AssetFilteringState _initState = AssetFilteringState::NONE;

	/**
	 * Initializes this class.
	 *
	 * Constructs and initializes member classes.
	 */
	cs_ret_code_t initInternal();

	// --------- Processing of accepted Assest ---------------

	/**
	 * Dispatches a TrackedEvent for the given advertisement.
	 */
	void handleScannedDevice(const scanned_device_t& asset);

	/**
	 * Check if the filter with given index accepts the device, call handleAcceptedAsset and.
	 * dispatches EVT_ASSET_ACCEPTED if so.
	 *
	 * Returns true if the filter accepts the device and the exclude flag is false.
	 */
	bool checkIfFilterAccepts(uint8_t filterIndex, const scanned_device_t& device);

	/**
	 * splits out into subhandlers based on filter output type.
	 * Performs desired actions for said output type.
	 */
	void handleAcceptedAsset(uint8_t filterIndex, AssetFilter filter, const scanned_device_t& asset);

	void handleAcceptedAssetOutputMac(uint8_t filterId, AssetFilter filter, const scanned_device_t& asset);
	void handleAcceptedAssetOutputAssetId(uint8_t filterId, AssetFilter filter, const scanned_device_t& asset);
	void handleAcceptedAssetOutputAssetIdNearest(uint8_t filterId, AssetFilter filter, const scanned_device_t& asset);

	/**
	 * Returns true if there is a filter that rejects this device.
	 * (Does not check if the filterstore is ready.)
	 */
	bool isAssetRejected(const scanned_device_t& device);

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};
