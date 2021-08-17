/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once
#include <cstdint>
#include <cstring>
#include <protocol/cs_CuckooFilterStructs.h>


// ------------------ Command protocol version ------------------
typedef uint8_t asset_filter_cmd_protocol_t;
constexpr asset_filter_cmd_protocol_t ASSET_FILTER_CMD_PROTOCOL_VERSION = 0;


// ------------------ Command values -----------------

struct __attribute__((__packed__)) asset_filter_cmd_upload_filter_t {
	asset_filter_cmd_protocol_t protocolVersion;
	uint8_t filterId;
	uint16_t chunkStartIndex;
	uint16_t totalSize;
	uint16_t chunkSize;
	uint8_t chunk[];  // flexible array, sizeof packet depends on chunkSize.
};

struct __attribute__((__packed__)) asset_filter_cmd_remove_filter_t {
	asset_filter_cmd_protocol_t protocolVersion;
	uint8_t filterId;
};

struct __attribute__((__packed__)) asset_filter_cmd_commit_filter_changes_t {
	asset_filter_cmd_protocol_t protocolVersion;
	uint16_t masterVersion;
	uint32_t masterCrc;
};

// ------------------ Return values ------------------

struct __attribute__((__packed__)) asset_filter_summary_t {
	uint8_t id;
	uint32_t crc;
};


struct __attribute__((__packed__)) asset_filter_cmd_get_filter_summaries_ret_t {
	asset_filter_cmd_protocol_t protocolVersion;
	uint16_t masterVersion;
	uint32_t masterCrc;
	uint16_t freeSpace;
	asset_filter_summary_t summaries[];
};

// ------------------ Filter format ------------------

enum class AssetFilterType : uint8_t {
	CuckooFilter = 0,
	ExactMatchFilter = 1,
};

enum class AssetFilterInputType : uint8_t {
	MacAddress       = 0,
	AdDataType       = 1,
	MaskedAdDataType = 2,
};

enum class AssetFilterOutputFormat : uint8_t {
	MacOverMesh          = 0,
	ShortAssetId         = 1,
	ShortAssetIdOverMesh = 2,
};

union __attribute__((__packed__)) asset_filter_flags_t {
	struct __attribute__((packed)) {
		bool exclude: 1;
	} flags;
	uint8_t asInt = 0;
};


struct __attribute__((__packed__)) ad_data_type_selector_t {
	uint8_t adDataType;
};

struct __attribute__((__packed__)) masked_ad_data_type_selector_t {
	uint8_t adDataType;
	uint32_t adDataMask;
};


struct __attribute__((__packed__)) short_asset_id_t {
	uint8_t data[3];

	// default implementation according to C++20
	bool operator==(const short_asset_id_t& other) const {
		return std::memcmp(data, other.data, sizeof(data)) == 0;
	}
};

constexpr short_asset_id_t INVALID_ASSET_ID { .data{0xff, 0xff, 0xff} };


struct __attribute__((__packed__)) asset_filters_version_t {
	uint16_t masterVersion = 0;
	uint32_t masterCrc;
};



