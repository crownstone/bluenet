/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 14, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <test/cs_TestCentral.h>
#include <ble/cs_BleCentral.h>
#include <ble/cs_UUID.h>
#include <cfg/cs_UuidConfig.h>
#include <events/cs_EventDispatcher.h>
#include <util/cs_Utils.h>

TestCentral::TestCentral() :
	_fwVersionHandle(BLE_GATT_HANDLE_INVALID),
	_controlHandle(BLE_GATT_HANDLE_INVALID)
{

}

void TestCentral::init() {
	EventDispatcher::getInstance().addListener(this);
}

void TestCentral::connect() {
	TYPIFY(CMD_BLE_CENTRAL_CONNECT) cmdPacket = {
			.address = {
					.address = { 0xEC, 0x22, 0x35, 0xEF, 0x60, 0xF4 },
					.addressType = CS_ADDRESS_TYPE_RANDOM_STATIC
			},
			.timeoutMs = 3000
	};
	_log(SERIAL_INFO, false, "Address: ");
	BLEutil::printAddress((uint8_t*)cmdPacket.address.address, BLE_GAP_ADDR_LEN, SERIAL_INFO);

	event_t cmdEvent(CS_TYPE::CMD_BLE_CENTRAL_CONNECT, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("connect: %u", cmdEvent.result.returnCode);
}

void TestCentral::discover() {
	TYPIFY(CMD_BLE_CENTRAL_DISCOVER) cmdPacket;
	UUID uuids[4];
	uuids[0].fromFullUuid(CROWNSTONE_UUID);
	uuids[1].fromFullUuid(SETUP_UUID);
	uuids[2].fromShortUuid(BLE_UUID_DEVICE_INFORMATION_SERVICE);
	uuids[3].fromShortUuid(0xFE59); // DFU service
	cmdPacket.uuids = uuids;
	cmdPacket.uuidCount = sizeof(uuids) / sizeof(uuids[0]);
	event_t cmdEvent(CS_TYPE::CMD_BLE_CENTRAL_DISCOVER, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("discovery: %u", cmdEvent.result.returnCode);
}

void TestCentral::onDiscovery(ble_central_discovery_t& packet) {
	LOGi("Discovered a service/char uuid=%u type=%u valueHandle=%u cccdHandle=%u",
			packet.uuid.getUuid().uuid,
			packet.uuid.getUuid().type,
			packet.valueHandle,
			packet.cccdHandle);

	UUID crownstoneUuid;
	crownstoneUuid.fromFullUuid(CROWNSTONE_UUID);
	UUID controlUuid;
	controlUuid.fromBaseUuid(crownstoneUuid, CONTROL_UUID);
	if (packet.uuid == controlUuid) {
		_controlHandle = packet.valueHandle;
		LOGi("Found control handle: %u", _controlHandle);
	}

	UUID firmwareVersionUuid;
	firmwareVersionUuid.fromShortUuid(BLE_UUID_FIRMWARE_REVISION_STRING_CHAR);
	ble_uuid_t uuid1 = firmwareVersionUuid.getUuid();
	ble_uuid_t uuid2 = packet.uuid.getUuid();
	LOGi("fw version uuid=%u type=%u memcmp=%u size=%u",
			firmwareVersionUuid.getUuid().uuid,
			firmwareVersionUuid.getUuid().type,
			memcmp(&uuid1, &uuid2, sizeof(ble_uuid_t)),
			sizeof(ble_uuid_t));
	if (packet.uuid == firmwareVersionUuid) {
		_fwVersionHandle = packet.valueHandle;
		LOGi("Found fw version handle: %u", _fwVersionHandle);
	}
}

void TestCentral::read() {
	TYPIFY(CMD_BLE_CENTRAL_READ) cmdPacket;
	cmdPacket.handle = _fwVersionHandle;
	event_t cmdEvent(CS_TYPE::CMD_BLE_CENTRAL_READ, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("read: %u", cmdEvent.result.returnCode);
}

void TestCentral::write() {
	uint8_t writeData[100];
	for (size_t i = 0; i < sizeof(writeData); ++i) {
		writeData[i] = i;
	}
	TYPIFY(CMD_BLE_CENTRAL_WRITE) cmdPacket;
	cmdPacket.handle = _controlHandle;
	cmdPacket.data = cs_data_t(writeData, sizeof(writeData));
	event_t cmdEvent(CS_TYPE::CMD_BLE_CENTRAL_WRITE, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("write: %u", cmdEvent.result.returnCode);
}

void TestCentral::disconnect() {
	event_t cmdEvent(CS_TYPE::CMD_BLE_CENTRAL_DISCONNECT);
	cmdEvent.dispatch();
	LOGi("disconnect: %u", cmdEvent.result.returnCode);
}

void TestCentral::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::CMD_CONTROL_CMD: {
			control_command_packet_t* packet = CS_TYPE_CAST(CMD_CONTROL_CMD, event.data);
			if (packet->type == CTRL_CMD_UART_MSG) {
				connect();
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			cs_ret_code_t result = *CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			LOGi("Connect result: %u", result);
			if (result == ERR_SUCCESS) {
				discover();
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY: {
			auto packet = CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY, event.data);
			onDiscovery(*packet);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT: {
			cs_ret_code_t result = *CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY_RESULT, event.data);
			LOGi("Discovery result: %u", result);
			if (result == ERR_SUCCESS) {
				read();
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_READ_RESULT: {
			auto packet = CS_TYPE_CAST(EVT_BLE_CENTRAL_READ_RESULT, event.data);
			LOGi("Read result: %u", packet->retCode);
			if (packet->retCode == ERR_SUCCESS) {
				_logArray(SERIAL_INFO, true, packet->data.data, packet->data.len);
				write();
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT: {
			cs_ret_code_t result = *CS_TYPE_CAST(EVT_BLE_CENTRAL_WRITE_RESULT, event.data);
			LOGi("Write result: %u", result);
			if (result == ERR_SUCCESS) {
				disconnect();
			}
			break;
		}


		default:
			break;
	}
}
