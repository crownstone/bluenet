/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#define DEVICE_UNDEF                   0
#define DEVICE_CROWNSTONE_PLUG         1
#define DEVICE_GUIDESTONE              2
#define DEVICE_CROWNSTONE_BUILTIN      3
#define DEVICE_CROWNSTONE_USB          4
#define DEVICE_CROWNSTONE_BUILTIN_ONE  5


#define IS_CROWNSTONE(a) (a==DEVICE_CROWNSTONE_PLUG || a==DEVICE_CROWNSTONE_BUILTIN || a==DEVICE_CROWNSTONE_BUILTIN_ONE)
