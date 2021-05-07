/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFiltering.h>
#include <util/cs_Utils.h>

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

	// loop over filters to check mac address
	for (size_t i = 0; i < _filterStore->getFilterCount(); ++i) {
		AssetFilter filter = _filterStore->getFilter(i);
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
			return;
		}
	}

	// TODO(#177858707): Add the AD Data loop for other filter inputTypes.
	// loop over filters fields to check addata fields
	//	// keeps fields as outer loop because that's more expensive to loop over.
	// 	// See BLEutil:findAdvType how to loop the fields.
	//	for (auto field: devicefields) {
	//		for(size_t i = 0; i < _parsingFiltersCount; ++i) {
	//			tracking_filter_t* filter = _parsingFilters[i];
	//			if(filter->metadata.inputType == FilterInputType::AdData) {
	//				// check each AD Data field for this filter
	//				// TODO: query filter for fingerprint
	//				TrackableEvent trackEvent;
	//				trackEvent.dispatch();
	//				return;
	//			}
	//		}
	//	}
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

	_log(SERIAL_DEBUG, false, "servicedata: ");
	_logArray(SERIAL_DEBUG, true, serviceData.data, serviceData.len);
}

