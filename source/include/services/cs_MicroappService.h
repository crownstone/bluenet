/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Characteristic.h>
#include <ble/cs_Service.h>

class MicroappService : public Service {
public:
	MicroappService(const UUID& uuid);
	void createCharacteristics();
};
