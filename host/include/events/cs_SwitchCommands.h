/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 22, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <common/cs_Types.h>
#include <events/cs_Event.h>

template<enum CS_TYPE T, class RET = void, class... ARGS>
RET sendCommand(ARGS...) {

}

template<>
void sendCommand<CS_TYPE::CMD_DIMMING_ALLOWED>(bool dimmingAllowed) {
	event_t evt(CS_TYPE::CMD_DIMMING_ALLOWED, &dimmingAllowed, sizeof(dimmingAllowed));
	evt.dispatch();
}


