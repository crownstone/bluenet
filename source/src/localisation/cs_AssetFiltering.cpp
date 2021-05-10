/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFiltering.h>
#include <util/cs_Utils.h>

#define LogLevelAssetFilteringDebug SERIAL_VERY_VERBOSE
#define LogAssetFilteringDebug LOGd
#define LogAssetFilteringWarn LOGw

cs_ret_code_t AssetFiltering::init() {
	_filterStore = new AssetFilterStore();
	if (_filterStore == nullptr) {
		return ERR_NO_SPACE;
	}
	cs_ret_code_t retCode = _filterStore->init();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	listen();
	return ERR_SUCCESS;
}

void AssetFiltering::handleScannedDevice(const scanned_device_t& device) {
	if (_filterStore->isInProgress()) {
		return;
	}

	for (size_t i = 0; i < _filterStore->getFilterCount(); ++i) {
		auto filter = AssetFilter(_filterStore[i]);
		if (filterInputResult(filter, device)) {
			processAcceptedAsset(filter,device);
		}
	}
}

void AssetFiltering::processAcceptedAsset(AssetFilter filter, const scanned_device_t& asset) {
	switch (filter.filterdata().metadata().outputType().outFormat()) {
		case AssetFilterOutputFormat::Mac: {
			mac_address_t mac;
			mac.data = asset.address;
			processAcceptedAsset(filter, asset, mac);
			break;
		}

		case AssetFilterOutputFormat::ShortAssetId: {
			filter.filterdata().filterdata().getCompressedFingerprint();
			break;
		}
	}
}

void AssetFiltering::processFilter(AssetFilter f, const scanned_device_t& device) {

	CuckooFilter cuckoo = filter.filterdata().filterdata();

	// check mac address for this filter
	if (*filter.filterdata().metadata().inputType().type() == AssetFilterInputType::MacAddress
		&& cuckoo.contains(device.address, MAC_ADDRESS_LEN)) {
		// TODO(#177858707):
		// - get fingerprint from filter instead of literal mac address.
		// - we should just pass on the device, right than copying rssi?
		LOGw("filter %u accepted adv from mac addres: %x:%x:%x:%x:%x:%x",
			 filter.runtimedata()->filterId,
			 device.address[0],
			 device.address[1],
			 device.address[2],
			 device.address[3],
			 device.address[4],
			 device.address[5]);  // TODO(#177858707) when in/out types is added, test with actual uuid

		TrackableEvent trackableEventData;
		trackableEventData.id   = TrackableId(device.address, MAC_ADDRESS_LEN);
		trackableEventData.rssi = device.rssi;

		event_t trackableEvent(CS_TYPE::EVT_TRACKABLE, &trackableEventData, sizeof(trackableEventData));
		trackableEvent.dispatch();
	}
}

//
//template<class ReturnType, class ExpressionType>
//ReturnType getFilterResult(AssetFilter f, const scanned_device_t& device) {
//
//}

bool filterInputResult(AssetFilter f, const scanned_device_t& device) {
	if (*f.filterdata().metadata().filterType() != AssetFilterType::CuckooFilter) {
		LogAssetFilteringWarn("Filtertype not implemented");
		return false;
	}

	CuckooFilter cuckoo = f.filterdata().filterdata();

	switch (f.filterdata().metadata().inputType().type()) {
		case AssetFilterInputType::MacAddress: {
			return cuckoo.contains(device.address, sizeof(device.address));
		}
		case AssetFilterInputType::AdDataType: {
			// selects the first found field of configured type and checks if that field's
			// data is contained in the filter. returns false if it can't be found.
			cs_data_t result                  = {};
			ad_data_type_selector_t* selector = f.filterdata().metadata().inputType().AdTypeField();
			assert(selector != nullptr, "Filter metadata type check failed");
			if (BLEutil::findAdvType(selector->adDataType, device.data, device.dataSize, &result) == ERR_SUCCESS) {
				return cuckoo.contains(result.data, result.len);
			}
			return false;
		}
		case AssetFilterInputType::MaskedAdDataType: {
			// selects the first found field of configured type and checks if that field's
			// data is contained in the filter. returns false if it can't be found.
			cs_data_t result                  = {};
			masked_ad_data_type_selector_t* selector = f.filterdata().metadata().inputType().AdTypeMasked();
			assert(selector != nullptr, "Filter metadata type check failed");
			if (BLEutil::findAdvType(selector->adDataType, device.data, device.dataSize, &result) == ERR_SUCCESS) {
				uint8_t buff[31] = {0};
				assert(result.len <=31, "advertisement length too big");

				// blank out bytes that aren't set in the mask
				for (size_t i = 0; i < result.len; i++){
					if(BLEutil::isBitSet(selector->adDataMask, i)) {
						buff[i] = result.data[i];
					} else {
						buff[i] = 0x00;
					}
				}

				return cuckoo.contains(buff, result.len);
			}
			return false;
		}
	}
}

void AssetFiltering::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto scannedDevice = CS_TYPE_CAST(EVT_DEVICE_SCANNED, evt.data);
			handleScannedDevice(*scannedDevice);
			break;
		}
		default:
			break;
	}
}

// ======================== Utils ========================

void AssetFiltering::logServiceData(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceData;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA, scannedDevice->data, scannedDevice->dataSize, &serviceData);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	_log(LogLevelAssetFilteringDebug, false, "servicedata: ");
	_logArray(LogLevelAssetFilteringDebug, true, serviceData.data, serviceData.len);
}

