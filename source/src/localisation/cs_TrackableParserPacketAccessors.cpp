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
	if (*type() == AdvertisementSubdataType::MaskedAdDataType) {
		return reinterpret_cast<masked_ad_data_type_selector_t*>(
				_data + sizeof(AdvertisementSubdataType));
	}
	return nullptr;
}

size_t AdvertisementSubdata::length() {
	switch (*type()) {
		case AdvertisementSubdataType::MacAddress:
			return sizeof(AdvertisementSubdataType) + 0;
		case AdvertisementSubdataType::AdDataType:
			return sizeof(AdvertisementSubdataType) + sizeof(ad_data_type_selector_t);
		case AdvertisementSubdataType::MaskedAdDataType:
			return sizeof(AdvertisementSubdataType) + sizeof(masked_ad_data_type_selector_t);
	}
	return 0;
}

// -------------------------------
// FilterOutputType implementation
// -------------------------------

FilterOutputFormat* FilterOutputType::outFormat() {
	return reinterpret_cast<FilterOutputFormat*>(_data + 0);
}

AdvertisementSubdata FilterOutputType::inFormat() {
	return AdvertisementSubdata(_data + sizeof(FilterOutputFormat));
}

size_t FilterOutputType::length() {
	switch(*outFormat()){
		case FilterOutputFormat::Mac: return sizeof(FilterOutputFormat);
		case FilterOutputFormat::ShortAssetId: return sizeof(FilterOutputFormat) + inFormat().length();
	}
	return 0;
}

// -------------------------------------
// TrackingFilterMetaData implementation
// -------------------------------------

FilterType* TrackingFilterMetadata::type() {
	return reinterpret_cast<FilterType*>(_data + 0);
}

uint8_t* TrackingFilterMetadata::profileId() {
	return _data + sizeof(FilterType);
}

AdvertisementSubdata TrackingFilterMetadata::inputType() {
	return AdvertisementSubdata(_data + sizeof(FilterType) + sizeof(uint8_t));
}

FilterOutputType TrackingFilterMetadata::outputType() {
	return FilterOutputType(
			_data + sizeof(FilterType) + sizeof(uint8_t) + inputType().length());
}

size_t TrackingFilterMetadata::length() {
	return sizeof(FilterType)
			+ sizeof(uint8_t)
			+ inputType().length()
			+ outputType().length();
}

// --------------------------------
// TrackinFilterData implementation
// --------------------------------

TrackingFilterMetadata TrackingFilterData::metadata() {
	return TrackingFilterMetadata(_data + 0);
}
CuckooFilter TrackingFilterData::filterdata() {
	return CuckooFilter(*reinterpret_cast<cuckoo_filter_data_t*>(
			_data + metadata().length()));
}

size_t TrackingFilterData::length() {
	return metadata().length() + filterdata().size();
}

// --------------------------------
// TrackinFilter implementation
// --------------------------------

tracking_filter_runtime_data_t* TrackingFilter::runtimedata() {
	return reinterpret_cast<tracking_filter_runtime_data_t*>(_data + 0);
}

TrackingFilterData TrackingFilter::filterdata() {
	return TrackingFilterData(_data + sizeof(tracking_filter_runtime_data_t));
}

size_t TrackingFilter::length() {
	return sizeof(tracking_filter_runtime_data_t) + filterdata().length();
}
