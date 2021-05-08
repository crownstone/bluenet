/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFilterPacketAccessors.h>


// -----------------------------------
// AdvertisementSubdata implementation
// -----------------------------------

AssetFilterInputType* AssetFilterInput::type() {
	return reinterpret_cast<AssetFilterInputType*>(_data + 0);
}

ad_data_type_selector_t* AssetFilterInput::AdTypeField() {
	if (*type() == AssetFilterInputType::AdDataType) {
		return reinterpret_cast<ad_data_type_selector_t*>(_data + sizeof(AssetFilterInputType));
	}
	return nullptr;
}

masked_ad_data_type_selector_t* AssetFilterInput::AdTypeMasked() {
	if (*type() == AssetFilterInputType::MaskedAdDataType) {
		return reinterpret_cast<masked_ad_data_type_selector_t*>(_data + sizeof(AssetFilterInputType));
	}
	return nullptr;
}

size_t AssetFilterInput::length() {
	switch (*type()) {
		case AssetFilterInputType::MacAddress:
			return sizeof(AssetFilterInputType) + 0;
		case AssetFilterInputType::AdDataType:
			return sizeof(AssetFilterInputType) + sizeof(ad_data_type_selector_t);
		case AssetFilterInputType::MaskedAdDataType:
			return sizeof(AssetFilterInputType) + sizeof(masked_ad_data_type_selector_t);
	}
	return 0;
}

// -------------------------------
// FilterOutputType implementation
// -------------------------------

AssetFilterOutputFormat* AssetFilterOutput::outFormat() {
	return reinterpret_cast<AssetFilterOutputFormat*>(_data + 0);
}

AssetFilterInput AssetFilterOutput::inFormat() {
	return AssetFilterInput(_data + sizeof(AssetFilterOutputFormat));
}

size_t AssetFilterOutput::length() {
	switch(*outFormat()){
		case AssetFilterOutputFormat::Mac: return sizeof(AssetFilterOutputFormat);
		case AssetFilterOutputFormat::ShortAssetId: return sizeof(AssetFilterOutputFormat) + inFormat().length();
	}
	return 0;
}

// -------------------------------------
// TrackingFilterMetaData implementation
// -------------------------------------

AssetFilterType* AssetFilterMetadata::filterType() {
	return reinterpret_cast<AssetFilterType*>(_data + 0);
}

uint8_t* AssetFilterMetadata::profileId() {
	return _data + sizeof(AssetFilterType);
}

AssetFilterInput AssetFilterMetadata::inputType() {
	return AssetFilterInput(_data + sizeof(AssetFilterType) + sizeof(uint8_t));
}

AssetFilterOutput AssetFilterMetadata::outputType() {
	return AssetFilterOutput(_data + sizeof(AssetFilterType) + sizeof(uint8_t) + inputType().length());
}

size_t AssetFilterMetadata::length() {
	return sizeof(AssetFilterType)
			+ sizeof(uint8_t)
			+ inputType().length()
			+ outputType().length();
}

// --------------------------------
// TrackinFilterData implementation
// --------------------------------

AssetFilterMetadata AssetFilterData::metadata() {
	return AssetFilterMetadata(_data + 0);
}
CuckooFilter AssetFilterData::filterdata() {
	return CuckooFilter(*reinterpret_cast<cuckoo_filter_data_t*>(_data + metadata().length()));
}

size_t AssetFilterData::length() {
	return metadata().length() + filterdata().size();
}

// --------------------------------
// TrackinFilter implementation
// --------------------------------

asset_filter_runtime_data_t* AssetFilter::runtimedata() {
	return reinterpret_cast<asset_filter_runtime_data_t*>(_data + 0);
}

AssetFilterData AssetFilter::filterdata() {
	return AssetFilterData(_data + sizeof(asset_filter_runtime_data_t));
}

size_t AssetFilter::length() {
	return sizeof(asset_filter_runtime_data_t) + filterdata().length();
}
