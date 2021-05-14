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
 * The sdk_config.h is a copy of SDK_15-3/config/nrf52832/config/sdk_config.h
 */

// Set to 1 to define legacy driver configs, instead of the new NRFX ones.
// This will overwrite some NRFX defines, via apply_old_config.h.
#define CS_DEFINE_LEGACY_NRF_DRIVERS_CONFIGS 0

// It is still the case that apply_old_config.h is applied...
// The problem is described at 
//   https://devzone.nordicsemi.com/f/nordic-q-a/60127/compare-sdk_config-files
// As soon as an old macro like TWI_ENABLED is defined in app_config.h (or anywhere else) it will lead to the
// NRFX_TWI_ENABLED macro to be disabled.
// Note that it does not matter if it is defined as 0, even worse. The default app_config.h will define it such even
// if it is not defined!

#define APP_SCHEDULER_ENABLED 1
#define APP_TIMER_ENABLED 1
#define APP_TIMER_CONFIG_OP_QUEUE_SIZE 40
#define APP_TIMER_CONFIG_USE_SCHEDULER 1

// <i> NRF_SDH_DISPATCH_MODEL_INTERRUPT: SoftDevice events are passed to the application from the interrupt context.
// <i> NRF_SDH_DISPATCH_MODEL_APPSH: SoftDevice events are scheduled using @ref app_scheduler.
// <i> NRF_SDH_DISPATCH_MODEL_POLLING: SoftDevice events are to be fetched manually.
// <0=> NRF_SDH_DISPATCH_MODEL_INTERRUPT
// <1=> NRF_SDH_DISPATCH_MODEL_APPSH
// <2=> NRF_SDH_DISPATCH_MODEL_POLLING
#define NRF_SDH_DISPATCH_MODEL 1

#define FDS_ENABLED 1
/**
 * Number of flash pages for FDS to use.
 * One page for GC
 * One page for config
 * One page for behaviour
 * One page for sure
 */
#define FDS_VIRTUAL_PAGES 4
// Virtual page size in words.
#define FDS_VIRTUAL_PAGE_SIZE 1024
#define FDS_VIRTUAL_PAGES_RESERVED 0
#define FDS_OP_QUEUE_SIZE 4
#define FDS_CRC_CHECK_ON_READ 1
#define FDS_MAX_USERS 2

#define NRF_FSTORAGE_ENABLED 1
#define NRF_FSTORAGE_SD_QUEUE_SIZE 4


#define HARDFAULT_HANDLER_ENABLED 1

#define APP_SCHEDULER_WITH_PROFILER 1


#if CS_SERIAL_NRF_LOG_ENABLED > 0
#define NRF_LOG_ENABLED 1
#else
#define NRF_LOG_ENABLED 0
#endif

//! Log data is buffered and can be processed in idle
#define NRF_LOG_DEFERRED 1
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_DEFAULT_LEVEL 4
#define NRF_SDH_SOC_LOG_LEVEL 4
#define NRF_SDH_BLE_LOG_LEVEL 4
#define NRF_SDH_LOG_LEVEL 4

#define NRF_LOG_USES_COLORS 1
#define NRF_LOG_WARNING_COLOR 4
#define NRF_LOG_USES_TIMESTAMP 0
#define NRF_FPRINTF_ENABLED 1
#define NRF_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF_ENABLED 1


#if CS_SERIAL_NRF_LOG_ENABLED == 1
#define NRF_LOG_BACKEND_RTT_ENABLED 1
#else
#define NRF_LOG_BACKEND_RTT_ENABLED 0
#endif

#if CS_SERIAL_NRF_LOG_ENABLED == 2
#define NRF_LOG_BACKEND_UART_ENABLED 1
#else
#define NRF_LOG_BACKEND_UART_ENABLED 0
#endif

#define NRF_LOG_BACKEND_UART_TX_PIN CS_SERIAL_NRF_LOG_PIN_TX

#if CS_SERIAL_NRF_LOG_ENABLED == 2
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


#define BLE_DB_DISCOVERY_ENABLED 1

#define NRF_SDH_BLE_ENABLED 1
#define NRF_SDH_BLE_PERIPHERAL_LINK_COUNT 1
#define NRF_SDH_BLE_CENTRAL_LINK_COUNT 1

// Though we can have 1 outgoing (central), and 1 incoming (peripheral) connection, we can't have them both at the same time, because we set total link count to 1.
// This means we should stop connectable advertisements before connecting to a peripheral.
#define NRF_SDH_BLE_TOTAL_LINK_COUNT 1

#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE 69 // Advised by mesh, see MESH_GATT_MTU_SIZE_MAX.
//#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE 72 // For microapps, we want a multiple of 4. Ok this doesn't make sense, as there's a L2CAP header of 3 bytes?
#define NRF_SDH_BLE_GATTS_ATTR_TAB_SIZE ATTR_TABLE_SIZE
#define NRF_SDH_BLE_VS_UUID_COUNT MAX_NUM_VS_SERVICES
/**
 * For iOS, the absence of the service changed characteristic makes it always discover services.
 */
#define NRF_SDH_BLE_SERVICE_CHANGED 1
#define NRF_SDH_BLE_OBSERVER_PRIO_LEVELS 2
#define NRF_SDH_ENABLED 1
#define NRF_SDH_REQ_OBSERVER_PRIO_LEVELS 1
#define NRF_SDH_STATE_OBSERVER_PRIO_LEVELS 1
#define NRF_SDH_BLE_STACK_OBSERVER_PRIO 1
#define NRF_SDH_SOC_ENABLED 1
#define NRF_SDH_SOC_OBSERVER_PRIO_LEVELS 1

// Required for FDS
#define CRC16_ENABLED 1
#define CRC32_ENABLED 1

// Used by cs_Comp
#define COMP_ENABLED 1



#if CS_DEFINE_LEGACY_NRF_DRIVERS_CONFIGS == 0
// NRFX_WDT_ENABLED is overwritten by apply_old_config.h
#define WDT_ENABLED 1
#endif

#define NRFX_WDT_ENABLED 1

// <1=> Run in SLEEP, Pause in HALT
// <8=> Pause in SLEEP, Run in HALT
// <9=> Run in SLEEP and HALT
// <0=> Pause in SLEEP and HALT
#ifdef DEBUG
#define NRFX_WDT_CONFIG_BEHAVIOUR 1
#else
#define NRFX_WDT_CONFIG_BEHAVIOUR 9
#endif

// <0=> Include WDT IRQ handling
// <1=> Remove WDT IRQ handling
#define NRFX_WDT_CONFIG_NO_IRQ 1

// Enable TWI
// Still TWI0_ENABLED rather than NRFX_TWI0_ENABLED which won't work
#ifdef BUILD_TWI
#define NRFX_TWI_ENABLED 1
#define NRFX_TWI0_ENABLED 1
#endif

#ifdef BUILD_GPIOTE
#define NRFX_GPIOTE_ENABLED 1
#endif

//#define NRFX_SAADC_ENABLED 1


//#define NRFX_RTC_ENABLED 1
//#define NRFX_RTC0_ENABLED 1
//#define NRFX_RTC1_ENABLED 1
//#define NRFX_RTC2_ENABLED 1


//#define NRFX_TIMER_ENABLED 1
//#define NRFX_TIMER0_ENABLED 1
//#define NRFX_TIMER1_ENABLED 1
//#define NRFX_TIMER2_ENABLED 1
//#define NRFX_TIMER3_ENABLED 1
//#define NRFX_TIMER4_ENABLED 1

//#define UART_ENABLED 1
//#define UART_EASY_DMA_SUPPORT 0
//#define UART0_ENABLED 1
//#define UART0_CONFIG_USE_EASY_DMA 0













/**
 * Settings below were missing from the sdk_config.h
 * They're copied from some example sdk_config.h
 */


// <o> NRF_LOG_BACKEND_RTT_TEMP_BUFFER_SIZE - Size of buffer for partially processed strings.
// <i> Size of the buffer is a trade-off between RAM usage and processing.
// <i> if buffer is smaller then strings will often be fragmented.
// <i> It is recommended to use size which will fit typical log and only the
// <i> longer one will be fragmented.
#ifndef NRF_LOG_BACKEND_RTT_TEMP_BUFFER_SIZE
#define NRF_LOG_BACKEND_RTT_TEMP_BUFFER_SIZE 64
#endif

// <o> NRF_LOG_BACKEND_RTT_TX_RETRY_DELAY_MS - Period before retrying writing to RTT
#ifndef NRF_LOG_BACKEND_RTT_TX_RETRY_DELAY_MS
#define NRF_LOG_BACKEND_RTT_TX_RETRY_DELAY_MS 1
#endif

// <o> NRF_LOG_BACKEND_RTT_TX_RETRY_CNT - Writing to RTT retries.
// <i> If RTT fails to accept any new data after retries
// <i> module assumes that host is not active and on next
// <i> request it will perform only one write attempt.
// <i> On successful writing, module assumes that host is active
// <i> and scheme with retry is applied again.
#ifndef NRF_LOG_BACKEND_RTT_TX_RETRY_CNT
#define NRF_LOG_BACKEND_RTT_TX_RETRY_CNT 3
#endif

// <o> SEGGER_RTT_CONFIG_BUFFER_SIZE_UP - Size of upstream buffer.
// <i> Note that either @ref NRF_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE
// <i> or this value is actually used. It depends on which one is bigger.
#ifndef SEGGER_RTT_CONFIG_BUFFER_SIZE_UP
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_UP 4096
#endif

// <o> SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS - Maximum number of upstream buffers.
#ifndef SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS
#define SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS 2
#endif

// <o> SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN - Size of downstream buffer.
#ifndef SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN 16
#endif

// <o> SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS - Maximum number of downstream buffers.
#ifndef SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS
#define SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS 2
#endif

// <o> SEGGER_RTT_CONFIG_DEFAULT_MODE  - RTT behavior if the buffer is full.
// <i> The following modes are supported:
// <i> - SKIP  - Do not block, output nothing.
// <i> - TRIM  - Do not block, output as much as fits.
// <i> - BLOCK - Wait until there is space in the buffer.
// <0=> SKIP
// <1=> TRIM
// <2=> BLOCK_IF_FIFO_FULL
#ifndef SEGGER_RTT_CONFIG_DEFAULT_MODE
#define SEGGER_RTT_CONFIG_DEFAULT_MODE 0
#endif

// <o> NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE - Size of buffer for partially processed strings.
// <i> Size of the buffer is a trade-off between RAM usage and processing.
// <i> if buffer is smaller then strings will often be fragmented.
// <i> It is recommended to use size which will fit typical log and only the
// <i> longer one will be fragmented.
#ifndef NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE
#define NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE 64
#endif
