/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Sep 23, 2015
 * License: LGPLv3+
 */
#pragma once

#define DEVICE_UNDEF               0
#define DEVICE_CROWNSTONE_PLUG     1
#define DEVICE_GUIDESTONE          2
#define DEVICE_CROWNSTONE_BUILTIN  3

#define IS_CROWNSTONE(a) a==DEVICE_CROWNSTONE_PLUG || a==DEVICE_CROWNSTONE_BUILTIN
