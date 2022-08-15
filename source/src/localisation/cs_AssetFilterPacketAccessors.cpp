/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFilterPacketAccessors.h>
#include <logging/cs_Logger.h>

#define LogAssetFilterPacketAccessorsWarn LOGw

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

bool AssetFilterInput::isValid() {
	switch (*type()) {
		case AssetFilterInputType::MacAddress:
		case AssetFilterInputType::AdDataType:
		case AssetFilterInputType::MaskedAdDataType: {
			return true;
		}
	}
	LogAssetFilterPacketAccessorsWarn("invalid assetfilter input type");
	return false;
}

size_t AssetFilterInput::length() {
	size_t len = sizeof(AssetFilterInputType);
	switch (*type()) {
		case AssetFilterInputType::MacAddress: {
			break;
		}
		case AssetFilterInputType::AdDataType: {
			len += sizeof(ad_data_type_selector_t);
			break;
		}
		case AssetFilterInputType::MaskedAdDataType: {
			len += sizeof(masked_ad_data_type_selector_t);
			break;
		}
	}
	return len;
}

AssetFilterOutputFormat* AssetFilterOutput::outFormat() {
	return reinterpret_cast<AssetFilterOutputFormat*>(_data + 0);
}

AssetFilterInput AssetFilterOutput::inFormat() {
	return AssetFilterInput(_data + sizeof(AssetFilterOutputFormat));
}

bool AssetFilterOutput::hasInFormat() {
	auto outform = *outFormat();

	if (outform == AssetFilterOutputFormat::AssetId) {
		return true;
	}
#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	if (outform == AssetFilterOutputFormat::AssetIdNearest) {
		return true;
	}
#endif

	return false;
}

bool AssetFilterOutput::isValid() {
	if (hasInFormat()) {
		if (inFormat().isValid() == false) {
			LogAssetFilterPacketAccessorsWarn("Invalid informat in filter output.");
			return false;
		}
		return true;
	}

	switch (*outFormat()) {
		case AssetFilterOutputFormat::Mac:
		case AssetFilterOutputFormat::AssetId:
		case AssetFilterOutputFormat::None:
#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
		case AssetFilterOutputFormat::AssetIdNearest:
#endif

			return true;
	}

	LogAssetFilterPacketAccessorsWarn("Unknown outFormat: %u", *outFormat());
	return false;
}

size_t AssetFilterOutput::length() {
	size_t len = sizeof(AssetFilterOutputFormat);

	if (hasInFormat()) {
		len += inFormat().length();
	}

	return len;
}

AssetFilterType* AssetFilterMetadata::filterType() {
	return reinterpret_cast<AssetFilterType*>(_data + 0);
}

asset_filter_flags_t* AssetFilterMetadata::flags() {
	return reinterpret_cast<asset_filter_flags_t*>(_data + sizeof(AssetFilterType));
}

uint8_t* AssetFilterMetadata::profileId() {
	return _data + sizeof(AssetFilterType) + sizeof(asset_filter_flags_t);
}

AssetFilterInput AssetFilterMetadata::inputType() {
	return AssetFilterInput(_data + sizeof(AssetFilterType) + sizeof(asset_filter_flags_t) + sizeof(uint8_t));
}

AssetFilterOutput AssetFilterMetadata::outputType() {
	return AssetFilterOutput(
			_data + sizeof(AssetFilterType) + sizeof(asset_filter_flags_t) + sizeof(uint8_t) + inputType().length());
}

bool AssetFilterMetadata::isValid() {
	return inputType().isValid() && outputType().isValid();
}

size_t AssetFilterMetadata::length() {
	return sizeof(AssetFilterType) + sizeof(asset_filter_flags_t) + sizeof(uint8_t) + inputType().length()
		   + outputType().length();
}

AssetFilterMetadata AssetFilterData::metadata() {
	return AssetFilterMetadata(_data + 0);
}

CuckooFilter AssetFilterData::cuckooFilter() {
	return CuckooFilter(reinterpret_cast<cuckoo_filter_data_t*>(_data + metadata().length()));
}

ExactMatchFilter AssetFilterData::exactMatchFilter() {
	return ExactMatchFilter(reinterpret_cast<exact_match_filter_data_t*>(_data + metadata().length()));
}

bool AssetFilterData::isValid() {
	if (metadata().isValid() == false) {
		LogAssetFilterPacketAccessorsWarn("Invalid metadata.");
		return false;
	}
	switch (*metadata().filterType()) {
		case AssetFilterType::CuckooFilter: {
			return cuckooFilter().isValid();
		}
		case AssetFilterType::ExactMatchFilter: {
			return exactMatchFilter().isValid();
		}
		default: {
			LogAssetFilterPacketAccessorsWarn("Unknown filterType: %u", *metadata().filterType());
			return false;
		}
	}
}

size_t AssetFilterData::length() {
	size_t len = metadata().length();
	switch (*metadata().filterType()) {
		case AssetFilterType::CuckooFilter: {
			len += cuckooFilter().size();
			break;
		}
		case AssetFilterType::ExactMatchFilter: {
			len += exactMatchFilter().size();
			break;
		}
		default: {
			break;
		}
	}
	return len;
}
