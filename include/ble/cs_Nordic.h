/**
 * @file
 * Import files from Nordic SDK.
 *
 * @authors Crownstone Team
 * @copyright Crownstone B.V.
 * @date Apr 21, 2015
 * @license LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

//! The header files for the SoftDevice. We're trying really hard to only them only here.
//! The user should not need to include files under a Nordic license.
//! This means that we can distribute our files in the end without the corresponding Nordic header files.
#include "ble_gap.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_gatts.h"
#include "ble_gatt.h"
#include "ble_hci.h"

//! Refering to files in the Nordic SDK
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"
#include "nrf_error.h"

#include "app_util.h"
//#include "app_timer.h"
#include "app_scheduler.h"
#if NORDIC_SDK_VERSION<=11
#include "app_timer_appsh.h"
#endif

#ifndef BOOTLOADER_COMPILATION
// Refering to driver files
#include "nrf_drv_saadc.h"
#include "nrf_timer.h"
#include "nrf_ppi.h"
#include "nrf_gpiote.h"
//#include "nrf_drv_timer.h"
//#include "nrf_drv_ppi.h"
//#include "nrf_drv_gpiote.h"
#endif

#undef APP_ERROR_CHECK // undefine again, we want to use our own macro defined in util/cs_BleError.h
#undef APP_ERROR_HANDLER

#include "softdevice_handler_appsh.h"
//#include "softdevice_handler.h"
#include "nrf_nvic.h"

#ifdef __cplusplus
}
#endif


