/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 14, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <test/cs_TestCrownstoneCentral.h>
#include <ble/cs_CrownstoneCentral.h>
#include <events/cs_EventDispatcher.h>
#include <util/cs_Utils.h>

TestCrownstoneCentral::TestCrownstoneCentral() {}

void TestCrownstoneCentral::init() {
	EventDispatcher::getInstance().addListener(this);
}

void TestCrownstoneCentral::connect() {
	TYPIFY(CMD_CS_CENTRAL_CONNECT) cmdPacket = {
			.address = {
					.address = { 0xEC, 0x22, 0x35, 0xEF, 0x60, 0xF4 },
					.addressType = CS_ADDRESS_TYPE_RANDOM_STATIC
			},
			.timeoutMs = 3000
	};
	_log(SERIAL_INFO, false, "Address: ");
	BLEutil::printAddress((uint8_t*)cmdPacket.address.address, BLE_GAP_ADDR_LEN, SERIAL_INFO);

	event_t cmdEvent(CS_TYPE::CMD_CS_CENTRAL_CONNECT, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("connect: %u", cmdEvent.result.returnCode);
}



void TestCrownstoneCentral::read() {
}

void TestCrownstoneCentral::write() {
	TYPIFY(CMD_CS_CENTRAL_WRITE) cmdPacket = {
			.commandType = CTRL_CMD_GET_MAC_ADDRESS,
			.data = cs_data_t()
	};

	event_t cmdEvent(CS_TYPE::CMD_CS_CENTRAL_WRITE, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("write: %u", cmdEvent.result.returnCode);
}

void TestCrownstoneCentral::disconnect() {
}

void TestCrownstoneCentral::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::CMD_CONTROL_CMD: {
			control_command_packet_t* packet = CS_TYPE_CAST(CMD_CONTROL_CMD, event.data);
			if (packet->type == CTRL_CMD_UART_MSG) {
				connect();
			}
			break;
		}
		case CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT: {
			cs_ret_code_t result = *CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
			LOGi("Connect result: %u", result);
			if (result == ERR_SUCCESS) {
				write();
			}
			break;
		}
		case CS_TYPE::EVT_CS_CENTRAL_WRITE_RESULT: {
			auto result = CS_TYPE_CAST(EVT_CS_CENTRAL_WRITE_RESULT, event.data);
			LOGi("Write result: %u", result->retCode);
			break;
		}

		default:
			break;
	}
}
