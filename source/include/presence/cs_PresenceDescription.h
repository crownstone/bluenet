/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

// this typedef is expected to expand to a full class when implementing 
// the presence detection.
// Current datatype: each bit indicates whether there is some user present
// in the room labeled with that bit index.
typedef uint64_t PresenceStateDescription;