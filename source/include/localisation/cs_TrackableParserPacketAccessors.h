/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_TrackableParserPackets.h>

class AdvertisementSubdata {
private:
	uint8_t* _data;  // byte representation of this object.

public:
	AdvertisementSubdata(uint8_t* data) : _data(data){}

	AdvertisementSubdataType*       type();
	ad_data_type_selector_t*        AdTypeField();  // returns nullptr unless type() == AdDataType
	masked_ad_data_type_selector_t* AdTypeMasked(); // returns nullptr unless type() == MaskedAdDataType

	/**
	 * Total length of the AdvertisementSubdata. Depends on type().
	 */
	size_t length();
};

/**
 * This is an accessor class that defines the packet structure.
 * and can deal with variable length packets 'in the middle'.
 */
class TrackingFilterMetaData {
private:
	uint8_t* _data;  // byte representation of this object.

public:
	TrackingFilterMetaData(uint8_t* data) : _data(data) {}

	FilterType*           type();
	uint8_t*              profileId();
	AdvertisementSubdata  inputType();
	filter_output_type_t* outputType();

	size_t length(); // TODO.
};
