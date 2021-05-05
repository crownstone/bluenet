/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once
#include <cstdint>
#include <protocol/cs_CuckooFilterStructs.h>


constexpr uint8_t TRACKABLE_PARSER_PROTOCOL_VERSION = 0;

// ------------------ command wrapper packet ------------------
struct __attribute__((__packed__)) trackable_parser_cmd_wrapper_t {
	uint8_t commandProtocolVersion;
	uint8_t payload[];
};


// ------------------ Command values -----------------

struct __attribute__((__packed__)) trackable_parser_cmd_upload_filter_t {
	uint8_t filterId;
	uint16_t chunkStartIndex;
	uint16_t totalSize;
	uint16_t chunkSize;
	uint8_t chunk[];  // flexible array, sizeof packet depends on chunkSize.
};

struct __attribute__((__packed__)) trackable_parser_cmd_remove_filter_t {
	uint8_t filterId;
};

struct __attribute__((__packed__)) trackable_parser_cmd_commit_filter_changes_t {
	uint16_t masterVersion;
	uint16_t masterCrc;
};

struct __attribute__((__packed__)) trackable_parser_cmd_get_filter_summaries_t {
	// empty
};

// ------------------ Return values ------------------

struct __attribute__((__packed__)) tracking_filter_summary_t {
	uint8_t id;
	uint8_t type;
	uint16_t crc;
};


struct __attribute__((__packed__)) trackable_parser_cmd_get_filter_summaries_ret_t {
	uint16_t masterVersion;
	uint16_t masterCrc;
	uint16_t freeSpace;
	tracking_filter_summary_t summaries[];
};

// ------------------ Filter format ------------------

enum class FilterType : uint8_t {
	CuckooFilter = 0,
};

enum class AdvertisementSubdataType : uint8_t {
	MacAddress       = 0,
	AdDataType       = 1,
	MaskedAdDataType = 2,
};

enum class FilterOutputFormat : uint8_t {
	Mac          = 0,
	ShortAssetId = 1,
};

struct __attribute__((__packed__)) ad_data_type_selector_t {
	uint8_t adDataType;
};

struct __attribute__((__packed__)) masked_ad_data_type_selector_t {
	uint8_t adDataType;
	uint32_t adDataMask;
};

struct __attribute__((__packed__)) filter_output_type_t {
	FilterOutputFormat out_format;
	uint8_t in_format[];
};


//struct __attribute__((__packed__)) advertisement_subdata_t {
//	AdvertisementSubdataType type;
//	uint8_t auxData[];
//};


// @Bart: this is a solution direction for those variable length headers
// that we need to be able to access the data in.

#include <optional>

class AdvertisementSubdata {
private:
	uint8_t (&_data)[];  // reference to an array containing the data
public:
	AdvertisementSubdata(uint8_t (&data)[]) : _data(data){}

	AdvertisementSubdataType&                      type();
	std::optional<ad_data_type_selector_t&>        AdTypeField();
	std::optional<masked_ad_data_type_selector_t&> AdTypeMasked();

	size_t length();
};

/**
 * This is an accessor class that defines the packet structure.
 * and can deal with variable length packets 'in the middle'.
 */
class TrackingFilterMetaData {
private:
	uint8_t (&_data)[];  // reference to an array containing the data
public:
	TrackingFilterMetaData(uint8_t (&data)[]) : _data(data) {}

	FilterType&           type();
	uint8_t&              profileId();
	AdvertisementSubdata  inputType();
	filter_output_type_t& outputType();

	size_t length(); // TODO.
};


// -----------------------------------
// AdvertisementSubdata implementation
// -----------------------------------

AdvertisementSubdataType& AdvertisementSubdata::type() {
	return *reinterpret_cast<AdvertisementSubdataType*>(_data + 0);
}

std::optional<ad_data_type_selector_t&> AdvertisementSubdata::AdTypeField() {
	if (type() == AdvertisementSubdataType::AdDataType) {
		// cast first byte to correct type and return
		return *reinterpret_cast<ad_data_type_selector_t*>(_data + 1);
	}
	return {};
}

std::optional<masked_ad_data_type_selector_t&> AdvertisementSubdata::AdTypeMasked() {
	if (type() == AdvertisementSubdataType::MaskedAdDataType) {
		// cast first byte to correct type and return
		return *reinterpret_cast<masked_ad_data_type_selector_t*>(_data + 1);
	}
	return {};
}

size_t AdvertisementSubdata::length() {
	switch(type()) {
		case AdvertisementSubdataType::MacAddress: return 1;
		case AdvertisementSubdataType::AdDataType: return 2;
		case AdvertisementSubdataType::MaskedAdDataType: return 6;
	}
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


