/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

extern "C" {
#include <nrf_mesh.h>
}

/**
 * Class that handles scans.
 *
 * Implements a callback `onScan` that listens for NRF_MESH_RX_SOURCE_SCANNER events,
 * transforms them the easier to manage scanned_device_t and then emits an event
 * of type EVT_DEVICE_SCANNED.
 */
class MeshScanner {
public:
	void onScan(const nrf_mesh_adv_packet_rx_data_t *scanData);
private:
	/**
	 * Variable to copy scanned data to, so that it doesn't get created on the stack all the time.
	 */
	scanned_device_t _scannedDevice = {0};
};
