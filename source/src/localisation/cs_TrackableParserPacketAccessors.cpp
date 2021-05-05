/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_TrackableParserPacketAccessors.h>


// -----------------------------------
// AdvertisementSubdata implementation
// -----------------------------------

AdvertisementSubdataType* AdvertisementSubdata::type() {
	return reinterpret_cast<AdvertisementSubdataType*>(_data + 0);
}

ad_data_type_selector_t* AdvertisementSubdata::AdTypeField() {
	if (*type() == AdvertisementSubdataType::AdDataType) {
		return reinterpret_cast<ad_data_type_selector_t*>(
				_data + sizeof(AdvertisementSubdataType));
	}
	return nullptr;
}

masked_ad_data_type_selector_t* AdvertisementSubdata::AdTypeMasked() {
	if (type() == AdvertisementSubdataType::MaskedAdDataType) {
		return reinterpret_cast<masked_ad_data_type_selector_t*>(
				_data + sizeof(AdvertisementSubdataType));
	}
	return nullptr;
}

size_t AdvertisementSubdata::length() {
	switch (type()) {
		case AdvertisementSubdataType::MacAddress:
			return sizeof(AdvertisementSubdataType) + 0;
		case AdvertisementSubdataType::AdDataType:
			return sizeof(AdvertisementSubdataType) + sizeof(ad_data_type_selector_t);
		case AdvertisementSubdataType::MaskedAdDataType:
			return sizeof(AdvertisementSubdataType) + sizeof(masked_ad_data_type_selector_t);
	}
	return 0;
}

// -------------------------------------
// TrackingFilterMetaData implementation
// -------------------------------------

FilterType& TrackingFilterMetaData::type() {
	return *reinterpret_cast<FilterType*>(_data + 0);
}

uint8_t& TrackingFilterMetaData::profileId() {
	return _data[1];
}

AdvertisementSubdata  TrackingFilterMetaData::inputType() {
	return AdvertisementSubdata(_data + 2);
}

filter_output_type_t& TrackingFilterMetaData::outputType() {
	return *reinterpret_cast<filter_output_type_t*>(_data + 2 + inputType().length());
}
