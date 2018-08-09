/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 9 Aug., 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Config.h>
#include <storage/cs_Settings.h>

#define FLOOR_DEFAULT            0
#define SCAN_FILTER_DEFAULT      0

void getDefaults(uint8_t configurationType, void* default_type, size_t & default_size);
