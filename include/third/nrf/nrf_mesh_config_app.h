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




/** Number of active servers.
 * Note: If the value of SERVER_NODE_COUNT is increased, you may need to scale up the the replay
 * protection list size (@ref REPLAY_CACHE_ENTRIES), by the appropriate amount, for the provisioner and
 * client examples. For the provisioner example to work as expected, its replay protection list size should
 * be greater than or equal to the total number of nodes it is going to configure after provisioning them.
 * The replay protection list size of the client example should be greater than or equal to the total
 * number of unicast addresses in the network that it can receive a message from.
 */
#define SERVER_NODE_COUNT (30)
#if SERVER_NODE_COUNT > 30
#error Maximum 30 servers currently supported by client example.
#endif

/** Number of active clients nodes. */
#define CLIENT_NODE_COUNT            (1)

/** Number of On-Off client models on the Switch Node */
#define CLIENT_MODEL_INSTANCE_COUNT  (2)

/** Number of group address being used in this example */
#define GROUP_ADDR_COUNT             (2)

/** Static authentication data */
#define STATIC_AUTH_DATA {0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F, 0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x5F, 0x31}

/** UUID prefix length */
#define NODE_UUID_PREFIX_LEN         (4)

/** UUID prefix length */
#define COMMON_UUID_PREFIX_LEN       (3)

/** Common UUID prefix for client nodes */
#define COMMON_CLIENT_UUID              0x00, 0x59, 0xCC

/** Light switch client UUID */
#define CLIENT_NODE_UUID_PREFIX         {COMMON_CLIENT_UUID, 0xAA}

/** Light switch level client UUID */
#define LEVEL_CLIENT_NODE_UUID_PREFIX   {COMMON_CLIENT_UUID, 0xBB}

/** Common UUID prefix for server nodes */
#define COMMON_SERVER_UUID              0x00, 0x59, 0x55

/** UUID prefix for other nodes */
#define SERVER_NODE_UUID_PREFIX         {COMMON_SERVER_UUID, 0xAA}

/** UUID prefix for other level server nodes */
#define LEVEL_SERVER_NODE_UUID_PREFIX   {COMMON_SERVER_UUID, 0xBB}



/** Enable logging module. */
#define NRF_MESH_LOG_ENABLE 0

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
//#define LOG_ENABLE_RTT 0


/**
 * @defgroup NRF_MESH_CONFIG_APP nRF Mesh app config
 *
 * Application side configuration file. Should be copied into every
 * application, and customized to fit its requirements.
 * @{
 */

/**
 * @defgroup DEVICE_CONFIG Device configuration
 *
 * @{
 */

#include "fds.h"
#include "fds_internal_defs.h"
/** Number of flash pages to be reserved between the flash manager recovery page and the bootloader.
 *  @note This value will be ignored if FLASH_MANAGER_RECOVERY_PAGE is set.
 */
#define FLASH_MANAGER_RECOVERY_PAGE_OFFSET_PAGES FDS_PHY_PAGES

/** Device company identifier. */
#define DEVICE_COMPANY_ID (ACCESS_COMPANY_ID_NORDIC)

/** Device product identifier. */
#define DEVICE_PRODUCT_ID (0x0000)

/** Device version identifier. */
#define DEVICE_VERSION_ID (0x0000)

/** @} end of DEVICE_CONFIG */

/**
 * @defgroup ACCESS_CONFIG Access layer configuration
 * @{
 */

/**
 * The default TTL value for the node.
 */
#define ACCESS_DEFAULT_TTL (SERVER_NODE_COUNT > NRF_MESH_TTL_MAX ? NRF_MESH_TTL_MAX : SERVER_NODE_COUNT)

/**
 * The number of models in the application.
 *
 * @note To fit the configuration and health models, this value must equal at least
 * the number of models needed by the application plus two.
 */
#define ACCESS_MODEL_COUNT (1 + /* Configuration server */  \
                            1 + /* Health server */  \
                            2 + /* Generic OnOff client (2 groups) */ \
                            2   /* Generic OnOff client (2 unicast) */)

/**
 * The number of elements in the application.
 *
 * @warning If the application is to support _multiple instances_ of the _same_ model, these instances
 * cannot be in the same element and a separate element is needed for each new instance of the same model.
 */
#define ACCESS_ELEMENT_COUNT (1 + CLIENT_MODEL_INSTANCE_COUNT) /* One element per Generic OnOff client instance */

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

/**
 * @defgroup ACCESS_RELIABLE_CONFIG Configuration of access layer reliable transfer
 * @{
 */

/** Number of the allowed parallel transfers (size of the internal context pool). */
#define ACCESS_RELIABLE_TRANSFER_COUNT (ACCESS_MODEL_COUNT)

/** @} end of ACCESS_RELIABLE_CONFIG */


/** @} end of ACCESS_CONFIG */


/**
 * @defgroup DSM_CONFIG Device State Manager configuration
 * Sizes for the internal storage of the Device State Manager.
 * @{
 */
/** Maximum number of subnetworks. */
#define DSM_SUBNET_MAX                                  (1)
/** Maximum number of applications. */
#define DSM_APP_MAX                                     (1)
/** Maximum number of device keys. */
#define DSM_DEVICE_MAX                                  (1)
/** Maximum number of virtual addresses. */
#define DSM_VIRTUAL_ADDR_MAX                            (1)
/** Maximum number of non-virtual addresses. One for each of the servers and a group address. */
#define DSM_NONVIRTUAL_ADDR_MAX                         (ACCESS_MODEL_COUNT + 1)
/** Number of flash pages reserved for the DSM storage. */
#define DSM_FLASH_PAGE_COUNT                            (1)
/** @} end of DSM_CONFIG */


/** @} */

#endif /* NRF_MESH_CONFIG_APP_H__ */
