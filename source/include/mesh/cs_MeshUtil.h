/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

extern "C" {
#include <access.h>

#include "nrf_mesh.h"
}

/**
 * Mesh utils with dependencies on mesh SDK.
 */
namespace MeshUtil {

/**
 * Get the RSSI from meta data.
 */
int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData);

uint8_t getChannel(const nrf_mesh_rx_metadata_t* metaData);

/**
 * Factory method because a constructor would pollute the struct.
 */
MeshMsgEvent fromAccessMessageRX(const access_message_rx_t& accessMsg);

/**
 * Print a mesh address.
 */
void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr);

}  // namespace MeshUtil
