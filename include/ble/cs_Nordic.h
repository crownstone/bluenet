/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

// The header files for the SoftDevice. We're trying really hard to only them only here.
// The user should not need to include files under a Nordic license.
// This means that we can distribute our files in the end without the corresponding Nordic header files.
#include "ble_gap.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_gatts.h"
#include "ble_gatt.h"

// Refering to files in the Nordic SDK
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"
#include "nrf_error.h"

#include "app_timer.h"
#undef APP_ERROR_CHECK // undefine again, we want to use our own macro defined in util/cs_BleError.h

#include "softdevice_handler.h"

#ifdef __cplusplus
}
#endif


