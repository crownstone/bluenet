/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

// These match the value in UICR, and are used for the hardware version string.
#define PRODUCT_UNKNOWN                  0
#define PRODUCT_DEV_BOARD                1
#define PRODUCT_CROWNSTONE_PLUG_ZERO     2
#define PRODUCT_CROWNSTONE_BUILTIN_ZERO  3
#define PRODUCT_GUIDESTONE               4
#define PRODUCT_CROWNSTONE_USB_DONGLE    5
#define PRODUCT_CROWNSTONE_BUILTIN_ONE   6
#define PRODUCT_CROWNSTONE_HUB           7
#define PRODUCT_CROWNSTONE_BUILTIN_TWO   8
#define PRODUCT_CROWNSTONE_PLUG_ONE      9

// These are used for service data.
#define DEVICE_UNDEF                   0
#define DEVICE_CROWNSTONE_PLUG         1
#define DEVICE_GUIDESTONE              2
#define DEVICE_CROWNSTONE_BUILTIN      3
#define DEVICE_CROWNSTONE_USB          4
#define DEVICE_CROWNSTONE_BUILTIN_ONE  5

#define DEVICE_CROWNSTONE_HUB          7
#define DEVICE_CROWNSTONE_BUILTIN_TWO  8
#define DEVICE_CROWNSTONE_PLUG_ONE     9
#define DEVICE_CROWNSTONE_OUTLET      10

#define IS_CROWNSTONE(a) ( \
		a==DEVICE_CROWNSTONE_PLUG || \
		a==DEVICE_CROWNSTONE_BUILTIN || \
		a==DEVICE_CROWNSTONE_BUILTIN_ONE || \
		a==DEVICE_CROWNSTONE_BUILTIN_TWO || \
		a==DEVICE_CROWNSTONE_OUTLET || \
		a==DEVICE_CROWNSTONE_PLUG_ONE || \
		a==DEVICE_CROWNSTONE_OUTLET \
		)
