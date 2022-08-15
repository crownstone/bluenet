/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 19, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_MultiSwitchHandler.h"

#include "events/cs_EventDispatcher.h"
#include "storage/cs_State.h"

MultiSwitchHandler::MultiSwitchHandler() {}

void MultiSwitchHandler::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownId, sizeof(_ownId));
	EventDispatcher::getInstance().addListener(this);
}

void MultiSwitchHandler::handleMultiSwitch(internal_multi_switch_item_t* item, cmd_source_with_counter_t& source) {
	if (item->id == _ownId) {
		TYPIFY(CMD_SWITCH)* eventData = &(item->cmd);
		event_t event(CS_TYPE::CMD_SWITCH, eventData, sizeof(TYPIFY(CMD_SWITCH)), source);
		EventDispatcher::getInstance().dispatch(event);
		return;
	}
	else {
		LOGd("send multi switch");
		TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH)* eventData = item;
		event_t event(
				CS_TYPE::CMD_SEND_MESH_MSG_MULTI_SWITCH,
				eventData,
				sizeof(TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH)),
				source);
		EventDispatcher::getInstance().dispatch(event);
		return;
	}
}

void MultiSwitchHandler::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::CMD_MULTI_SWITCH: {
			handleMultiSwitch((TYPIFY(CMD_MULTI_SWITCH)*)event.data, event.source);
			break;
		}
		default: break;
	}
}
