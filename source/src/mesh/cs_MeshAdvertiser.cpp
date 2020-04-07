/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 26 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshAdvertiser.h>
#include <storage/cs_State.h>
#include <util/cs_BleError.h>
#include <time/cs_SystemTime.h>
#include <util/cs_Math.h>

//void txComplete(advertiser_t * p_adv, nrf_mesh_tx_token_t token, timestamp_t timestamp) {
//	LOGd("tx complete int=%u", BLEutil::getInterruptLevel());
//}

void MeshAdvertiser::init() {
	if (_advertiser != NULL) {
		return;
	}
	static advertiser_t advertiser;
	_advertiser = &advertiser;
	_buffer = (uint8_t*)malloc(MESH_ADVERTISER_BUF_SIZE);
	if (_buffer == NULL) {
		APP_ERROR_CHECK(ERR_NO_SPACE);
	}
	advertiser_instance_init(_advertiser, NULL, _buffer, MESH_ADVERTISER_BUF_SIZE);
//	advertiser_instance_init(_advertiser, txComplete, _buffer, MESH_ADVERTISER_BUF_SIZE);

	listen();
}

void MeshAdvertiser::setMacAddress(uint8_t* macAddress) {
	ble_gap_addr_t address;
//	address.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
	address.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
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

void MeshAdvertiser::start() {
	advertiser_enable(_advertiser);
}

void MeshAdvertiser::stop() {
	advertiser_disable(_advertiser);
}

void MeshAdvertiser::advertiseIbeacon(uint8_t ibeaconIndex) {
	_ibeaconConfigId = ibeaconIndex;
	updateIbeacon();
}

void MeshAdvertiser::updateIbeacon() {
	TYPIFY(CONFIG_IBEACON_MAJOR) major;
	TYPIFY(CONFIG_IBEACON_MINOR) minor;
	TYPIFY(CONFIG_IBEACON_UUID) uuid;
	TYPIFY(CONFIG_IBEACON_TXPOWER) rssi;

	{
		cs_state_data_t data(CS_TYPE::CONFIG_IBEACON_MAJOR,   _ibeaconConfigId, (uint8_t*)&major, sizeof(major));
		if (State::getInstance().get(data) != ERR_SUCCESS) {
			return;
		}
	}
	{
		cs_state_data_t data(CS_TYPE::CONFIG_IBEACON_MINOR,   _ibeaconConfigId, (uint8_t*)&minor, sizeof(minor));
		if (State::getInstance().get(data) != ERR_SUCCESS) {
			return;
		}
	}
	{
		cs_state_data_t data(CS_TYPE::CONFIG_IBEACON_UUID,    _ibeaconConfigId, (uint8_t*)(uuid.uuid128), sizeof(uuid.uuid128));
		if (State::getInstance().get(data) != ERR_SUCCESS) {
			return;
		}
	}
	{
		cs_state_data_t data(CS_TYPE::CONFIG_IBEACON_TXPOWER, _ibeaconConfigId, (uint8_t*)&rssi, sizeof(rssi));
		if (State::getInstance().get(data) != ERR_SUCCESS) {
			return;
		}
	}

//	State::getInstance().get(CS_TYPE::CONFIG_IBEACON_MAJOR, &major, sizeof(major));
//	State::getInstance().get(CS_TYPE::CONFIG_IBEACON_MINOR, &minor, sizeof(minor));
//	State::getInstance().get(CS_TYPE::CONFIG_IBEACON_UUID, uuid.uuid128, sizeof(uuid.uuid128));
//	State::getInstance().get(CS_TYPE::CONFIG_IBEACON_TXPOWER, &rssi, sizeof(rssi));

//	State::getInstance().get(cs_state_data_t(CS_TYPE::CONFIG_IBEACON_MAJOR,   ibeaconIndex, (uint8_t*)&major, sizeof(major)));
//	State::getInstance().get(cs_state_data_t(CS_TYPE::CONFIG_IBEACON_MINOR,   ibeaconIndex, (uint8_t*)&minor, sizeof(minor)));
//	State::getInstance().get(cs_state_data_t(CS_TYPE::CONFIG_IBEACON_UUID,    ibeaconIndex, (uint8_t*)uuid.uuid128, sizeof(uuid.uuid128)));
//	State::getInstance().get(cs_state_data_t(CS_TYPE::CONFIG_IBEACON_TXPOWER, ibeaconIndex, (uint8_t*)&rssi, sizeof(rssi)));

	IBeacon beacon(uuid, major, minor, rssi);
	LOGd("Advertise ibeacon uuid=[%x %x .. %x] major=%u, minor=%u, rssi_on_1m=%i", uuid.uuid128[0], uuid.uuid128[1], uuid.uuid128[15], major, minor, rssi);
	advertise(&beacon);
}

/**
 * Advertise iBeacon data.
 *
 * See https://github.com/crownstone/bluenet/blob/master/docs/PROTOCOL.md#ibeacon-advertisement-packet
 */
void MeshAdvertiser::advertise(IBeacon* ibeacon) {
	if (_advPacket != NULL) {
		// See https://devzone.nordicsemi.com/f/nordic-q-a/58658/mesh-advertiser-crash-when-calling-advertiser_packet_discard
//		advertiser_packet_discard(_advertiser, _advPacket);
//		_advPacket = NULL;
		stop();
		memcpy(&(_advPacket->packet.payload[7]), ibeacon->getArray(), ibeacon->size());
		start();
		return;
	}
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
	advertiser_packet_send(_advertiser, _advPacket);
}

cs_ret_code_t MeshAdvertiser::handleSetIbeaconConfig(ibeacon_config_id_packet_t* packet) {
	LOGi("handleSetIbeaconConfig id=%u timestamp=%u interval=%u", packet->ibeaconConfigId, packet->timestamp, packet->interval);
	if (packet->ibeaconConfigId >= num_ibeacon_config_ids) {
		return ERR_WRONG_PARAMETER;
	}
	if (packet->interval == 0 && packet->timestamp == 0) {
		// Set config id now, and clear entry.
		_ibeaconConfigId = packet->ibeaconConfigId;
		_ibeaconInterval[packet->ibeaconConfigId].interval = 0;
		_ibeaconInterval[packet->ibeaconConfigId].timestamp = 0;
		updateIbeacon();
	}
	else {
		_ibeaconInterval[packet->ibeaconConfigId] = *packet;
	}
	return ERR_SUCCESS;
}

void MeshAdvertiser::handleTime(uint32_t now) {
	for (uint8_t i = 0; i < num_ibeacon_config_ids; ++i) {
		if (now >= _ibeaconInterval[i].timestamp) {
			if (_ibeaconInterval[i].interval == 0) {
				if (_ibeaconInterval[i].timestamp == 0) {
					// Empty entry.
					break;
				}
				// Set config id, and clear entry.
				_ibeaconConfigId = i;
				_ibeaconInterval[i].interval = 0;
				_ibeaconInterval[i].timestamp = 0;
				updateIbeacon();
			}
			else if (CsMath::mod((_ibeaconInterval[i].timestamp - now), _ibeaconInterval[i].interval) == 0)	{
				_ibeaconConfigId = i;
				updateIbeacon();
				return;
			}
		}
	}
}

void MeshAdvertiser::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::CONFIG_IBEACON_MAJOR: {
//			uint16_t* major = reinterpret_cast<TYPIFY(CONFIG_IBEACON_MAJOR)*>(event.data);
			updateIbeacon();
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_MINOR: {
//			uint16_t* minor = reinterpret_cast<TYPIFY(CONFIG_IBEACON_MINOR)*>(event.data);
			updateIbeacon();
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_UUID: {
//			cs_uuid128_t* uuid = reinterpret_cast<TYPIFY(CONFIG_IBEACON_UUID)*>(event.data);
			updateIbeacon();
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_TXPOWER: {
//			int8_t* txPower = reinterpret_cast<TYPIFY(CONFIG_IBEACON_TXPOWER)*>(event.data);
			updateIbeacon();
			break;
		}
		case CS_TYPE::STATE_TIME: {
			uint32_t* timestamp = (TYPIFY(STATE_TIME)*) event.data;
			handleTime(*timestamp);
			break;
		}
		case CS_TYPE::CMD_SET_IBEACON_CONFIG_ID: {
			auto packet = (TYPIFY(CMD_SET_IBEACON_CONFIG_ID)*) event.data;
			event.result.returnCode = handleSetIbeaconConfig(packet);
			break;
		}
		default:
			break;
	}
}
