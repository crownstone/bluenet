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
	uint8_t flags;
	uint16_t version;
	uint16_t crc;
};

struct __attribute__((__packed__)) trackable_parser_cmd_upload_filter_ret_t {};

struct __attribute__((__packed__)) trackable_parser_cmd_remove_filter_ret_t {};

struct __attribute__((__packed__)) trackable_parser_cmd_commit_filter_changes_ret_t {};

struct __attribute__((__packed__)) trackable_parser_cmd_get_filter_summaries_ret_t {
	uint16_t masterVersion;
	uint16_t masterCrc;
	uint16_t freeSpace;
	tracking_filter_summary_t summaries[];
};

// ------------------ Filter format ------------------

/**
 * Determines whether the filter has mac or advertisement data as input.
 */
enum class FilterInputType : uint8_t {
	MacAddress = 0,
	AdData     = 1,
};

/**
 * Data associated to a parsing filter that is persisted to flash.
 */
struct __attribute__((__packed__)) tracking_filter_meta_data_t {
   public:
	// sync info
	uint8_t protocol;  // determines implementation type of filter
	uint16_t version;

	uint8_t profileId;  // devices passing the filter are assigned this profile id
	FilterInputType inputType;

	// sync state
	union {
		uint8_t flags;
		struct {
			uint8_t isActive : 1;
		} bits;
	};
};
