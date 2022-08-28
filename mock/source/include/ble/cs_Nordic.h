/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#ifndef __ALIGN
#define __ALIGN(n) __attribute__((aligned(n)))
#endif

#include <nrf.h>

#define BLE_GAP_PASSKEY_LEN 6

#if __clang__
#define STRINGIFY(str) #str
#else
#define STRINGIFY(str) str
#endif
