/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_StaticConfig.h>
#include <ipc/cs_IpcRamData.h>
#include <ipc/cs_IpcRamDataContents.h>

// Check if contents are not too large.
static_assert(sizeof(bluenet_ipc_data_payload_t) == BLUENET_IPC_RAM_DATA_ITEM_SIZE, "Wrong IPC ram data payload size.");

// Check if IPC data fits in section.
static_assert(sizeof(bluenet_ipc_ram_data_t) <= g_RAM_BLUENET_IPC_LENGTH, "IPC ram data does not fit in section.");
