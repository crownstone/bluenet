/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 26 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "mesh/cs_MeshAdvertiser.h"
#include "storage/cs_State.h"
#include "util/cs_BleError.h"

//void txComplete(advertiser_t * p_adv, nrf_mesh_tx_token_t token, timestamp_t timestamp) {
//	LOGd("tx complete int=%u", BLEutil::getInterruptLevel());
//}

void MeshAdvertiser::init() {
	static advertiser_t advertiser;
	_advertiser = &advertiser;
	_buffer = (uint8_t*)malloc(MESH_ADVERTISER_BUF_SIZE);
	if (_buffer == NULL) {
		APP_ERROR_CHECK(ERR_NO_SPACE);
	}
	advertiser_instance_init(_advertiser, NULL, _buffer, MESH_ADVERTISER_BUF_SIZE);
//	advertiser_instance_init(_advertiser, txComplete, _buffer, MESH_ADVERTISER_BUF_SIZE);
}

void MeshAdvertiser::setMacAddress(uint8_t* macAddress) {
	ble_gap_addr_t address;
	address.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
//	address.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
	address.addr_id_peer = 0;
	memcpy(address.addr, macAddress, BLE_GAP_ADDR_LEN);
	advertiser_address_set(_advertiser, &address);
}

void MeshAdvertiser::setInterval(uint32_t intervalMs) {
	advertiser_interval_set(_advertiser, intervalMs);
}

void MeshAdvertiser::setTxPower(int8_t power) {
	radio_tx_power_t txPower;
	switch (power) {
	case -40: case -20: case -16: case -12: case -8: case -4: case 0: case 4:
		txPower = (radio_tx_power_t)power;
		break;
	default:
		LOGe("invalid TX power: %i", power);
		return;
	}
	advertiser_tx_power_set(_advertiser, txPower);
}

/**
 * Fill advertising packet with iBeacon data.
 *
 * See https://github.com/crownstone/bluenet/blob/master/docs/PROTOCOL.md#ibeacon-advertisement-packet
 */
void MeshAdvertiser::setIbeaconData(IBeacon* ibeacon) {
	_advPacket = advertiser_packet_alloc(_advertiser, 7 + ibeacon->size());
	_advPacket->packet.payload[0] = 0x02; // Length of next AD
	_advPacket->packet.payload[1] = 0x01; // Type: flags
	_advPacket->packet.payload[2] = 0x06; // Flags
	_advPacket->packet.payload[3] = 0x1A; // Length of next AD
	_advPacket->packet.payload[4] = 0xFF; // Type: manufacturer data
	_advPacket->packet.payload[5] = 0x4C; // Company id
	_advPacket->packet.payload[6] = 0x00;
//	_advPacket[7] = 0x02; // Apple payload type: iBeacon
//	_advPacket[8] = 0x15; // iBeacon payload length
//	memcpy(&(_advPacket[9]), ) // iBeacon UUID
	memcpy(&(_advPacket->packet.payload[7]), ibeacon->getArray(), ibeacon->size());
	_advPacket->config.repeats = ADVERTISER_REPEAT_INFINITE;
}

void MeshAdvertiser::start() {
	advertiser_enable(_advertiser);
	if (_advPacket != NULL) {
		advertiser_packet_send(_advertiser, _advPacket);
	}
	else {
		LOGe("No advPacket");
	}
}
