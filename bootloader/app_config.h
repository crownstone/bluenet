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


#define NRF_DFU_APP_DATA_AREA_SIZE (6*4096)


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
