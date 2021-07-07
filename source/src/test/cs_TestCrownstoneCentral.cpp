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
#include <storage/cs_State.h>
#include <drivers/cs_RNG.h>

TestCrownstoneCentral::TestCrownstoneCentral() {}

void TestCrownstoneCentral::init() {
	listen();
}

void TestCrownstoneCentral::connect() {
	TYPIFY(CMD_CS_CENTRAL_CONNECT) cmdPacket = {
			.stoneId = 36,
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

void TestCrownstoneCentral::writeGetMac() {
	TYPIFY(CMD_CS_CENTRAL_WRITE) cmdPacket = {
			.commandType = CTRL_CMD_GET_MAC_ADDRESS,
			.data = cs_data_t()
	};

	event_t cmdEvent(CS_TYPE::CMD_CS_CENTRAL_WRITE, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("writeGetMac: %u", cmdEvent.result.returnCode);
}

void TestCrownstoneCentral::writeSetup() {
	setup_data_t setupData;
	setupData.stoneId = RNG::getInstance().getRandom8();
	State::getInstance().get(CS_TYPE::CONFIG_SPHERE_ID,        &(setupData.sphereId),            sizeof(setupData.sphereId));
	State::getInstance().get(CS_TYPE::CONFIG_KEY_ADMIN,        &(setupData.adminKey),            sizeof(setupData.adminKey));
	State::getInstance().get(CS_TYPE::CONFIG_KEY_MEMBER,       &(setupData.memberKey),           sizeof(setupData.memberKey));
	State::getInstance().get(CS_TYPE::CONFIG_KEY_BASIC,        &(setupData.basicKey),            sizeof(setupData.basicKey));
	State::getInstance().get(CS_TYPE::CONFIG_KEY_SERVICE_DATA, &(setupData.serviceDataKey),      sizeof(setupData.serviceDataKey));
	State::getInstance().get(CS_TYPE::CONFIG_KEY_LOCALIZATION, &(setupData.localizationKey),     sizeof(setupData.localizationKey));
	RNG::fillBuffer(setupData.meshDeviceKey, sizeof(setupData.meshDeviceKey));
	State::getInstance().get(CS_TYPE::CONFIG_MESH_APP_KEY,     &(setupData.meshAppKey),          sizeof(setupData.meshAppKey));
	State::getInstance().get(CS_TYPE::CONFIG_MESH_NET_KEY,     &(setupData.meshNetKey),          sizeof(setupData.meshNetKey));
	State::getInstance().get(CS_TYPE::CONFIG_IBEACON_UUID,     &(setupData.ibeaconUuid.uuid128), sizeof(setupData.ibeaconUuid.uuid128));
	setupData.ibeaconMajor = RNG::getInstance().getRandom16();
	setupData.ibeaconMinor = RNG::getInstance().getRandom16();

	TYPIFY(CMD_CS_CENTRAL_WRITE) cmdPacket = {
			.commandType = CTRL_CMD_SETUP,
			.data = cs_data_t(reinterpret_cast<uint8_t*>(&setupData), sizeof(setupData))
	};

	event_t cmdEvent(CS_TYPE::CMD_CS_CENTRAL_WRITE, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("writeSetup: %u", cmdEvent.result.returnCode);
}

void TestCrownstoneCentral::writeGetPowerSamples() {
	cs_power_samples_request_t request = {
			.type = 2, // Now filtered
			.index = 0 // Voltage
	};

	TYPIFY(CMD_CS_CENTRAL_WRITE) cmdPacket = {
			.commandType = CTRL_CMD_GET_POWER_SAMPLES,
			.data = cs_data_t(reinterpret_cast<uint8_t*>(&request), sizeof(request))
	};

	event_t cmdEvent(CS_TYPE::CMD_CS_CENTRAL_WRITE, &cmdPacket, sizeof(cmdPacket));
	cmdEvent.dispatch();
	LOGi("writeGetPowerSamples: %u", cmdEvent.result.returnCode);
}


void TestCrownstoneCentral::disconnect() {
	event_t cmdEvent(CS_TYPE::CMD_CS_CENTRAL_DISCONNECT);
	cmdEvent.dispatch();
	LOGi("disconnect: %u", cmdEvent.result.returnCode);
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
				_writeStep = 0;
				writeGetMac();
			}
			break;
		}
		case CS_TYPE::EVT_CS_CENTRAL_WRITE_RESULT: {
			auto result = CS_TYPE_CAST(EVT_CS_CENTRAL_WRITE_RESULT, event.data);
			LOGi("Write result: %u", result->writeRetCode);
			cs_ret_code_t replyRetCode = ERR_UNSPECIFIED;
			if (result->result.isInitialized()) {
				_log(SERIAL_INFO, false, "Reply: protocol=%u type=%u result=%u data=",
						result->result.getProtocolVersion(),
						result->result.getType(),
						result->result.getResult());
				_logArray(SERIAL_INFO, true, result->result.getPayload().data, result->result.getPayload().len);
				replyRetCode = result->result.getResult();
			}

			switch (_writeStep) {
				case 0: {
					writeGetPowerSamples();
					break;
				}
				case 1: {
					writeSetup();
					break;
				}
				case 2: {
					// The setup command gives 2 results: first one tells us to wait for another result.
					if (replyRetCode != ERR_WAIT_FOR_SUCCESS) {
						disconnect();
					}
					break;
				}
				case 3: {
					disconnect();
				}
			}
			_writeStep++;
			break;
		}

		default:
			break;
	}
}
