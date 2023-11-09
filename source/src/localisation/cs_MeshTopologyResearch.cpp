/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <localisation/cs_MeshTopologyResearch.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>

MeshTopologyResearch::MeshTopologyResearch() {}

void MeshTopologyResearch::init() {
	TYPIFY(CONFIG_CROWNSTONE_ID) state_id;
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &state_id, sizeof(state_id));

	listen();
}
