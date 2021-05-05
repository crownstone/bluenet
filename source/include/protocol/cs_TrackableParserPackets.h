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

struct __attribute__((__packed__)) advertisement_subdata_t {
	AdvertisementSubdataType type;
	uint8_t auxData[];
};

struct __attribute__((__packed__)) filter_output_type_t {
	FilterOutputFormat out_format;
	uint8_t in_format[];
};

struct __attribute__((__packed__)) filter_input_type_t {
	advertisement_subdata_t format;
};





