/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 28 May, 2019
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#include <cs_config.h>

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

#define NRF_DFU_APP_ACCEPT_SAME_VERSION 1

#define NRF_DFU_REQUIRE_SIGNED_APP_UPDATE 0

/**
 * Reserved number of flash pages used for app data.
 * 8 for FDS.
 * 1 for IPC.
 * 4 For mesh:
 * See how_to_nordicSDK.md and https://devzone.nordicsemi.com/f/nordic-q-a/42632/mesh-sdk---nrf52840-flash-page-issue
 * 1 for mesh access (ACCESS_FLASH_PAGE_COUNT).
 * 1 for mesh dsm (DSM_FLASH_PAGE_COUNT).
 * 1 for mesh net state (NET_FLASH_PAGE_COUNT).
 * 1 for mesh defrag/garbage collection.
 */
#define NRF_DFU_APP_DATA_AREA_SIZE (0x1000 *                  \
	(                                                         \
		CS_FDS_VIRTUAL_PAGES +                                \
		CS_FDS_VIRTUAL_PAGES_RESERVED_BEFORE +                \
		CS_FDS_VIRTUAL_PAGES_RESERVED_AFTER  +                \
		(BUILD_MICROAPP_SUPPORT ? FLASH_MICROAPP_PAGES : 0) + \
		(BUILD_P2P_DFU ? 1 : 0)                               \
	))
//#define NRF_DFU_APP_DATA_AREA_SIZE ((8+1+4)*0x1000)

//! Device information service.
#define BLE_DIS_ENABLED 1

/**
 * By default, the bootloader changes MAC address when in bootloader mode.
 * This prevents connect issues, where services are cached etc.
 * But, since iOS devices don't get to see the MAC address, they don't know which device is in bootloader mode.
 * So we adjusted the code to make the bootloader not change MAC address with this define, so that it's easy to find.
 */
#define CS_DFU_CHANGE_MAC_ADDRESS 0

/**
 * For iOS, the absence of the service changed characteristic makes it always discover services.
 */
#define NRF_SDH_BLE_SERVICE_CHANGED 1

#if CS_SERIAL_BOOTLOADER_NRF_LOG_ENABLED > 0
#define NRF_LOG_ENABLED 1
#else
#define NRF_LOG_ENABLED 0
#endif

//! Log data is buffered and can be processed in idle
#define NRF_LOG_DEFERRED 0
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_DEFAULT_LEVEL 4
#define NRF_SDH_SOC_LOG_LEVEL 3
#define NRF_SDH_BLE_LOG_LEVEL 3
#define NRF_SDH_LOG_LEVEL 3

#define NRF_LOG_USES_COLORS 1
#define NRF_LOG_WARNING_COLOR 4
#define NRF_LOG_USES_TIMESTAMP 0
#define NRF_FPRINTF_ENABLED 1
#define NRF_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF_ENABLED 1

#if CS_SERIAL_BOOTLOADER_NRF_LOG_ENABLED == 1
#define NRF_LOG_BACKEND_RTT_ENABLED 1
#else
#define NRF_LOG_BACKEND_RTT_ENABLED 0
#endif

#if CS_SERIAL_BOOTLOADER_NRF_LOG_ENABLED == 2
#define NRF_LOG_BACKEND_UART_ENABLED 1
#else
#define NRF_LOG_BACKEND_UART_ENABLED 0
#endif

#define NRF_LOG_BACKEND_UART_TX_PIN CS_SERIAL_NRF_LOG_PIN_TX

#if CS_SERIAL_BOOTLOADER_NRF_LOG_ENABLED == 2
// UARTE_ENABLED is overwritten by apply_old_config.h
#define UARTE_ENABLED 1
#define UART0_ENABLED 1
#define UART_ENABLED 1
// It wouldn't compile when using UARTE, so use normal UART instead.
#define UART_LEGACY_SUPPORT 1
#define UART_EASY_DMA_SUPPORT 0
#define NRFX_UARTE_DEFAULT_CONFIG_HWFC 0
#define NRFX_UARTE_DEFAULT_CONFIG_PARITY 0
#endif

// <323584=> 1200 baud
// <643072=> 2400 baud
// <1290240=> 4800 baud
// <2576384=> 9600 baud
// <3862528=> 14400 baud
// <5152768=> 19200 baud
// <7716864=> 28800 baud
// <10289152=> 38400 baud
// <15400960=> 57600 baud
// <20615168=> 76800 baud
// <30801920=> 115200 baud
// <61865984=> 230400 baud
// <67108864=> 250000 baud
// <121634816=> 460800 baud
// <251658240=> 921600 baud
// <268435456=> 1000000 baud
#define NRF_LOG_BACKEND_UART_BAUDRATE 61865984

/**
 * Settings below were missing from the sdk_config.h
 * They're copied from some example sdk_config.h
 */

// <o> NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE - Size of buffer for partially processed strings.
// <i> Size of the buffer is a trade-off between RAM usage and processing.
// <i> if buffer is smaller then strings will often be fragmented.
// <i> It is recommended to use size which will fit typical log and only the
// <i> longer one will be fragmented.
#ifndef NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE
#define NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE 64
#endif

// <o> UART_DEFAULT_CONFIG_HWFC  - Hardware Flow Control
// <0=> Disabled
// <1=> Enabled
#ifndef UART_DEFAULT_CONFIG_HWFC
#define UART_DEFAULT_CONFIG_HWFC 0
#endif

// <o> UART_DEFAULT_CONFIG_PARITY  - Parity
// <0=> Excluded
// <14=> Included
#ifndef UART_DEFAULT_CONFIG_PARITY
#define UART_DEFAULT_CONFIG_PARITY 0
#endif

// <o> UART_DEFAULT_CONFIG_BAUDRATE  - Default Baudrate
// <323584=> 1200 baud
// <643072=> 2400 baud
// <1290240=> 4800 baud
// <2576384=> 9600 baud
// <3862528=> 14400 baud
// <5152768=> 19200 baud
// <7716864=> 28800 baud
// <10289152=> 38400 baud
// <15400960=> 57600 baud
// <20615168=> 76800 baud
// <30801920=> 115200 baud
// <61865984=> 230400 baud
// <67108864=> 250000 baud
// <121634816=> 460800 baud
// <251658240=> 921600 baud
// <268435456=> 1000000 baud
#ifndef UART_DEFAULT_CONFIG_BAUDRATE
#define UART_DEFAULT_CONFIG_BAUDRATE 30801920
#endif

// <i> Priorities 0,2 (nRF51) and 0,1,4,5 (nRF52) are reserved for SoftDevice
// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3
// <4=> 4
// <5=> 5
// <6=> 6
// <7=> 7
#ifndef UART_DEFAULT_CONFIG_IRQ_PRIORITY
#define UART_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif
