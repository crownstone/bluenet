/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_TrackableParserPackets.h>
#include <protocol/cs_CuckooFilterStructs.h>

/**
 * This file defines several accessor classes for structures that
 * relate to the trackable parser. They are constructed using a buffer pointer
 * and provide read/write access to the buffer according to the protocol.
 */


class AdvertisementSubdata {
private:
	uint8_t* _data;  // byte representation of this object.

public:
	AdvertisementSubdata(uint8_t* data) : _data(data){}

	AdvertisementSubdataType*       type();
	ad_data_type_selector_t*        AdTypeField();  // returns nullptr unless *type() == AdDataType
	masked_ad_data_type_selector_t* AdTypeMasked(); // returns nullptr unless *type() == MaskedAdDataType

	/**
	 * Total length of the AdvertisementSubdata. Depends on type().
	 */
	size_t length();
};

class FilterOutputType {
private:
	uint8_t* _data;

public:
	FilterOutputType(uint8_t* data) : _data(data) {}

	FilterOutputFormat* outFormat();
	AdvertisementSubdata inFormat(); // invalid unless *outFormat() == ShortAssetId

	size_t length();
};


class TrackingFilterMetadata {
private:
	uint8_t* _data;  // byte representation of this object.

public:
	TrackingFilterMetadata(uint8_t* data) : _data(data) {}

	FilterType*           type();
	uint8_t*              profileId();
	AdvertisementSubdata  inputType();
	FilterOutputType outputType();

	size_t length();
};

class TrackingFilterData {
private:
	uint8_t* _data;  // byte representation of this object.

public:
	TrackingFilterData(uint8_t* data) : _data(data) {}

	TrackingFilterMetadata metadata();
	CuckooFilter filterdata(); // invalid unless *metadata().type() == FilterType::CukooFilter
};
