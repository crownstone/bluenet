/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr. 21, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <app_util_platform.h>
typedef uint32_t ret_code_t;

// The header files in the Nordic SDK. Keep all the includes here, not dispersed over all the header files.
#include <app_scheduler.h>
#include <app_util.h>
#include <ble.h>
#include <ble_advdata.h>
#include <ble_gap.h>
#include <ble_gatt.h>
#include <ble_gattc.h>
#include <ble_gatts.h>
#include <ble_hci.h>
#include <ble_srv_common.h>
#include <sdk_config.h>
#if NORDIC_SDK_VERSION > 15
#include <cmsis_compiler.h>
#endif
#include <fds.h>
#include <nrf.h>
#include <nrf_delay.h>
#include <nrf_error.h>
#include <nrf_fstorage.h>
#include <nrf_gpio.h>
#include <nrf_gpiote.h>
#include <nrf_nvic.h>
#include <nrf_power.h>
#include <nrf_sdh.h>
#include <nrf_sdh_ble.h>
#include <nrf_sdh_soc.h>
#include <nrf_sdm.h>

// legacy, should be replaced by nrfx_comp.h (see below)
#include <nrf_comp.h>

// legacy #include <nrf_drv_uart.h>
#include <nrfx_glue.h>
//#include <nordic_common.h>
//#include <nrf_serial.h>
#include <nrfx_comp.h>
//#include <nrf_drv_comp.h>
#include <ble_db_discovery.h>
#include <nrfx_wdt.h>

#if BUILD_TWI == 1
#include <nrfx_twi.h>
#endif

#if BUILD_GPIOTE == 1
#include <nrfx_gpiote.h>
#endif

#if CS_SERIAL_NRF_LOG_ENABLED > 0
#include <nrf_log_ctrl.h>
#include <nrf_log_default_backends.h>
#include <nrfx_log.h>
#endif

#ifndef BOOTLOADER_COMPILATION
#include <crc16.h>
#include <crc32.h>
#include <nrf_gpiote.h>
#include <nrf_ppi.h>
#include <nrf_saadc.h>
#include <nrf_timer.h>
#include <nrf_uart.h>
#endif

#ifdef __cplusplus
}
#endif
