/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <sdk_config.h>
#include <ble_types.h>
#include <nrf.h>
#include <sdk_errors.h>

#define BLE_GAP_PASSKEY_LEN 6

#ifndef __ALIGN
#define __ALIGN(n) __attribute__((aligned(n)))
#endif

#ifndef STRINGIFY
#if __clang__
#define STRINGIFY(str) #str
#else
#define STRINGIFY(str) str
#endif
#endif  // ndef STRINGIFY
