/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 28 May, 2019
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

/**
 * Use this config file to overwrite values in sdk_config.h.
 *
 * The sdk_config.h is a copy of SDK_15-3/examples/dfu/secure_bootloader/pca10040_ble_debug/config/sdk_config.h
 */


#define NRF_BL_DFU_ENTER_METHOD_BUTTON 0
#define NRF_BL_DFU_ENTER_METHOD_PINRESET 0
#define NRF_BL_DFU_ENTER_METHOD_GPREGRET 1
#define NRF_BL_DFU_ENTER_METHOD_BUTTONLESS 0


#define NRF_DFU_IN_APP 0
#define NRF_DFU_SAVE_PROGRESS_IN_FLASH 0

#define NRF_DFU_APP_DOWNGRADE_PREVENTION 0


#define NRF_DFU_REQUIRE_SIGNED_APP_UPDATE 0

/**
 * Reserved number of flash pages used for app data.
 * 4 for FDS (FDS_PHY_PAGES).
 * See how_to_nordicSDK.md and https://devzone.nordicsemi.com/f/nordic-q-a/42632/mesh-sdk---nrf52840-flash-page-issue
 * 1 for mesh access (ACCESS_FLASH_PAGE_COUNT).
 * 1 for mesh dsm (DSM_FLASH_PAGE_COUNT).
 * 1 for mesh defrag/garbage collection.
 * 1 for mesh recovery?.
 */
#define NRF_DFU_APP_DATA_AREA_SIZE ((4+4)*4096)

//! Device information service.
#define BLE_DIS_ENABLED 1

#if CS_SERIAL_NRF_LOG_ENABLED == 1
#define NRF_LOG_BACKEND_RTT_ENABLED 1
#else
#define NRF_LOG_BACKEND_RTT_ENABLED 0
#endif


#if CS_SERIAL_NRF_LOG_ENABLED > 0
#define NRF_LOG_ENABLED 1
#else
#define NRF_LOG_ENABLED 0
#endif
