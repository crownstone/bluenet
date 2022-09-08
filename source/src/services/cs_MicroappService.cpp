/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_UuidConfig.h>
#include <logging/cs_Logger.h>
#include <services/cs_MicroappService.h>
#include <util/cs_Utils.h>

MicroappService::MicroappService(const UUID& uuid) {
	setUUID(uuid);
}

void MicroappService::createCharacteristics() {}
