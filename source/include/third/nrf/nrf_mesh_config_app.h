/* Copyright (c) 2010 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_MESH_CONFIG_APP_H__
#define NRF_MESH_CONFIG_APP_H__

#include "sdk_config.h"
#include "fds.h"
#include "fds_internal_defs.h"
#include "cfg/cs_Config.h"

// See more options in nrf_mesh_config_*.h




/** Enable logging module. */
#define NRF_MESH_LOG_ENABLE NRF_LOG_BACKEND_RTT_ENABLED

/** Default log level. Messages with lower criticality is filtered. */
// LOG_LEVEL_ASSERT ( 0) /**< Log level for assertions */
// LOG_LEVEL_ERROR  ( 1) /**< Log level for error messages. */
// LOG_LEVEL_WARN   ( 2) /**< Log level for warning messages. */
// LOG_LEVEL_REPORT ( 3) /**< Log level for report messages. */
// LOG_LEVEL_INFO   ( 4) /**< Log level for information messages. */
// LOG_LEVEL_DBG1   ( 5) /**< Log level for debug messages (debug level 1). */
// LOG_LEVEL_DBG2   ( 6) /**< Log level for debug messages (debug level 2). */
// LOG_LEVEL_DBG3   ( 7) /**< Log level for debug messages (debug level 3). */
// EVT_LEVEL_BASE   ( 8) /**< Base level for event logging. For internal use only. */
// EVT_LEVEL_ERROR  ( 9) /**< Critical error event logging level. For internal use only. */
// EVT_LEVEL_INFO   (10) /**< Normal event logging level. For internal use only. */
// EVT_LEVEL_DATA   (11) /**< Event data logging level. For internal use only. */
#define LOG_LEVEL_DEFAULT 7

/** Enable logging with RTT callback. */
#define LOG_ENABLE_RTT NRF_LOG_BACKEND_RTT_ENABLED



/** Relay feature */
#define MESH_FEATURE_RELAY_ENABLED (1)

/**
 * Enable persistent storage.
 */
#if MESH_PERSISTENT_STORAGE == 1
#define PERSISTENT_STORAGE 1
#else
#define PERSISTENT_STORAGE 0
#endif

#if MESH_PERSISTENT_STORAGE == 2
#define MESH_EXTERNAL_PERSISTENT_STORAGE 1
#else
#define MESH_EXTERNAL_PERSISTENT_STORAGE 0
#endif

/**
 * Enable active scanning.
 */
#define SCANNER_ACTIVE_SCANNING 1

/** Device company identifier. */
#define DEVICE_COMPANY_ID (CROWNSTONE_COMPANY_ID)

/** Device product identifier. */
#define DEVICE_PRODUCT_ID (0x0000)

/** Device version identifier. */
#define DEVICE_VERSION_ID (0x0000)

/**
 * Number of entries in the replay protection cache.
 *
 * @note The number of entries in the replay protection list directly limits the number of elements
 * a node can receive messages from on the current IV index. This means if your device has a replay
 * protection list with 40 entries, a message from a 41st unicast address (element )will be dropped
 * by the transport layer.
 *
 * @note The replay protection list size *does not* affect the node's ability to relay messages.
 *
 * @note This number is indicated in the device composition data of the node and provisioner can
 * make use of this information to prevent unwarranted filling of the replay list on a given node in
 * a mesh network.
 */
#define REPLAY_CACHE_ENTRIES 255

/**
 * The default TTL value for the node.
 */
#define ACCESS_DEFAULT_TTL (CS_MESH_DEFAULT_TTL)

/**
 * The number of models in the application.
 *
 * @note To fit the configuration and health models, this value must equal at least
 * the number of models needed by the application plus two.
 */
#define ACCESS_MODEL_COUNT (1 /* Configuration server */  \
                            + 1 /* Health server */  \
                            + 1 /* Crownstone multicast model */  \
                            + 1 /* Crownstone multicast acked model */  \
                            + 1   /* Crownstone unicast model */ \
                            + 1   /* Crownstone multicast neighbours model */)

/**
 * The number of elements in the application.
 *
 * @warning If the application is to support _multiple instances_ of the _same_ model, these instances
 * cannot be in the same element and a separate element is needed for each new instance of the same model.
 */
#define ACCESS_ELEMENT_COUNT (1)

/**
 * The number of allocated subscription lists for the application.
 *
 * @note This value must equal @ref ACCESS_MODEL_COUNT minus the number of
 * models operating on shared states.
 */
#define ACCESS_SUBSCRIPTION_LIST_COUNT (ACCESS_MODEL_COUNT)

/**
 * The number of pages of flash storage reserved for the access layer for persistent data storage.
 */
#define ACCESS_FLASH_PAGE_COUNT (1)

/** Number of the allowed parallel transfers (size of the internal context pool). */
#define ACCESS_RELIABLE_TRANSFER_COUNT (1 /* Configuration server */  \
                                        + 1 /* Health server */  \
                                        + 1 /* Crownstone unicast model */)

/** Define for acknowledging message transaction timeout, in micro seconds. */
#define MODEL_ACKNOWLEDGED_TRANSACTION_TIMEOUT  (SEC_TO_US(3))



/** Maximum number of subnetworks. */
//#define DSM_SUBNET_MAX                                  (1)
#define DSM_SUBNET_MAX                                  (4)

/** Maximum number of applications. */
#define DSM_APP_MAX                                     (1)
//#define DSM_APP_MAX                                     (8)

/** Maximum number of device keys. */
#define DSM_DEVICE_MAX                                  (1)

/** Maximum number of virtual addresses. */
#define DSM_VIRTUAL_ADDR_MAX                            (2)

/** Maximum number of non-virtual addresses. One for each of the servers and a group address.
 * - Generic OnOff publication
 * - Health publication
 * - Subscription address
 */
#define DSM_NONVIRTUAL_ADDR_MAX                         (ACCESS_MODEL_COUNT + 1)

/** Number of flash pages reserved for the DSM storage. */
#define DSM_FLASH_PAGE_COUNT                            (1)



/** Number of flash pages to be reserved between the flash manager recovery page and the bootloader.
 *  @note This value will be ignored if FLASH_MANAGER_RECOVERY_PAGE is set.
 */
//#define FLASH_MANAGER_RECOVERY_PAGE_OFFSET_PAGES        (FDS_PHY_PAGES)
// We reserve a few pages for future expansion of FDS pages.
#define FLASH_MANAGER_RECOVERY_PAGE_OFFSET_PAGES        (2 + FDS_PHY_PAGES)



#endif /* NRF_MESH_CONFIG_APP_H__ */
