/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <presence/cs_PresenceHandler.h>

uint64_t roomBitmaskSingle() {
    return 0b001;
}
uint64_t roomBitmaskMulti() {
    return 0b011;
}

PresenceStateDescription present() {
    TestAccess<PresenceStateDescription> testAccessPresenceDesc;
    testAccessPresenceDesc._bitmask |= roomBitmaskSingle();
    return testAccessPresenceDesc.get();
}

PresenceStateDescription absent() {
    TestAccess<PresenceStateDescription> testAccessPresenceDesc;
    testAccessPresenceDesc._bitmask = 0;
    return testAccessPresenceDesc.get();
}
