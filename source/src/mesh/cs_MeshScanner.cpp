/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshScanner.h>
#include <structs/cs_PacketsInternal.h>

/**
 * Variable to copy scanned data to, so that it doesn't get created on the stack all the time.
 * Made static, since the callback isn't part of the class.
 */
static scanned_device_t _scannedDevice;

void MeshScanner::onScan(const nrf_mesh_adv_packet_rx_data_t *scanData) {
	switch (p_rx_data->p_metadata->source) {
	case NRF_MESH_RX_SOURCE_SCANNER:{
//	    timestamp_t timestamp; /**< Device local timestamp of the start of the advertisement header of the packet in microseconds. */
//	    uint32_t access_addr; /**< Access address the packet was received on. */
//	    uint8_t  channel; /**< Channel the packet was received on. */
//	    int8_t   rssi; /**< RSSI value of the received packet. */
//	    ble_gap_addr_t adv_addr; /**< Advertisement address in the packet. */
//	    uint8_t adv_type;  /**< BLE GAP advertising type. */

//		const uint8_t* addr = p_rx_data->p_metadata->params.scanner.adv_addr.addr;
//		const uint8_t* p = p_rx_data->p_payload;
//		if (p[0] == 0x15 && p[1] == 0x16 && p[2] == 0x01 && p[3] == 0xC0 && p[4] == 0x05) {
////		if (p[1] == 0xFF && p[2] == 0xCD && p[3] == 0xAB) {
////		if (addr[5] == 0xE7 && addr[4] == 0x09 && addr[3] == 0x62) { // E7:09:62:02:91:3D
//			LOGd("Mesh scan: address=%02X:%02X:%02X:%02X:%02X:%02X type=%u rssi=%i chan=%u", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], p_rx_data->p_metadata->params.scanner.adv_type, p_rx_data->p_metadata->params.scanner.rssi, p_rx_data->p_metadata->params.scanner.channel);
//			LOGd("  adv_type=%u len=%u data=", p_rx_data->adv_type, p_rx_data->length);
//			BLEutil::printArray(p_rx_data->p_payload, p_rx_data->length);
//		}
		memcpy(_scannedDevice.address, p_rx_data->p_metadata->params.scanner.adv_addr.addr, sizeof(_scannedDevice.address)); // TODO: check addr_type and addr_id_peer
		_scannedDevice.addressType = (p_rx_data->p_metadata->params.scanner.adv_addr.addr_type & 0x7F) & ((p_rx_data->p_metadata->params.scanner.adv_addr.addr_id_peer & 0x01) << 7);
		_scannedDevice.rssi = p_rx_data->p_metadata->params.scanner.rssi;
		_scannedDevice.channel = p_rx_data->p_metadata->params.scanner.channel;
		_scannedDevice.dataSize = p_rx_data->length;
		_scannedDevice.data = (uint8_t*)(p_rx_data->p_payload);
		event_t event(CS_TYPE::EVT_DEVICE_SCANNED, (void*)&_scannedDevice, sizeof(_scannedDevice));
		event.dispatch();

//#if CS_SERIAL_NRF_LOG_ENABLED == 1
//		const uint8_t* addr = p_rx_data->p_metadata->params.scanner.adv_addr.addr;
//		__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "scanned: %02X:%02X:%02X:%02X:%02X:%02X type=%u chan=%u\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], p_rx_data->p_metadata->params.scanner.adv_type, p_rx_data->p_metadata->params.scanner.channel);
//#endif

		break;
	}
	case NRF_MESH_RX_SOURCE_GATT:

		break;
//	case NRF_MESH_RX_SOURCE_FRIEND:
//
//		break;
//	case NRF_MESH_RX_SOURCE_LOW_POWER:
//
//		break;
	case NRF_MESH_RX_SOURCE_INSTABURST:

		break;
	case NRF_MESH_RX_SOURCE_LOOPBACK:
		break;
	}
}
