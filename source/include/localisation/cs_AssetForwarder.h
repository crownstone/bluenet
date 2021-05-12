/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_AssetHandler.h>

class AssetForwarder : public AssetHandlerMac {
public:
	virtual void handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset);
};
