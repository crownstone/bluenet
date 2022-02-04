/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <mesh/cs_MeshScanner.h>
#include <structs/cs_PacketsInternal.h>

#include <cstring>

void MeshScanner::onScan(const nrf_mesh_adv_packet_rx_data_t *scanData) {
	switch (scanData->p_metadata->source) {
		case NRF_MESH_RX_SOURCE_SCANNER: {

			// Ignore when the CPU is busy.
			uint16_t schedulerSpace = app_sched_queue_space_get();
			if (schedulerSpace < MIN_SCHEDULER_FREE) {
				LOGMeshWarning("Dropping scanned device: scheduler is quite full.");
				return;
			}

			scanned_device_t _scannedDevice = {0};
			
			auto &scanner = scanData->p_metadata->params.scanner;

			memcpy(_scannedDevice.address, scanner.adv_addr.addr, sizeof(_scannedDevice.address));

			_scannedDevice.resolvedPrivateAddress = scanner.adv_addr.addr_id_peer;
			_scannedDevice.addressType = scanner.adv_addr.addr_type;
			_scannedDevice.rssi = scanner.rssi;
			//_scannedDevice.setId = scanner.set_id;
			_scannedDevice.advType = scanner.adv_type;
			_scannedDevice.channel = scanner.channel;
			_scannedDevice.dataSize = scanData->length;
			_scannedDevice.data = const_cast<uint8_t*>(scanData->p_payload);

			event_t event(CS_TYPE::EVT_DEVICE_SCANNED, static_cast<void*>(&_scannedDevice), sizeof(_scannedDevice));
			event.dispatch();
			break;
		}
		case NRF_MESH_RX_SOURCE_GATT:
			break;
		case NRF_MESH_RX_SOURCE_INSTABURST:
			break;
		case NRF_MESH_RX_SOURCE_LOOPBACK:
			break;
	}
}
