/** Store "micro" apps in flash.
 *
 * Write a file to flash. This class listens to events and stores them in flash. The location where it is stored in 
 * flash is separate from the bluenet binary. It can be seen as a DFU process where bluenet functions as a bootloader
 * for yet other binaries that we call here microapps.
 *
 * A typical microapp can be a small binary compiled with Arduino conventions for the Crownstone architecture.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include "nrf_fstorage_sd.h"

#include <algorithm>
#include <ble/cs_UUID.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <microapp/cs_Microapp.h>
#include <microapp/cs_MicroappProtocol.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

Microapp::Microapp(): EventListener() {

	EventDispatcher::getInstance().addListener(this);

	_prevMessage.protocol = 0;
	_prevMessage.app_id = 0;
	_prevMessage.payload = 0;
	_prevMessage.repeat = 0;

	_enabled = false;
	_debug = true;
}

uint16_t Microapp::init() {
	uint32_t err_code;

	MicroappStorage & storage = MicroappStorage::getInstance();
	storage.init();

	// Set callback handler in IPC ram
	MicroappProtocol & protocol = MicroappProtocol::getInstance();
	protocol.setIpcRam();

	// Actually, first check if there's anything there?
	// TODO

	// Check for valid app on boot
	err_code = storage.validateApp();
	if (err_code == ERR_SUCCESS) {
		LOGi("Set app valid");
		storage.setAppValid();
	} else {
		LOGi("Checksum error");
	}

	if (err_code != NRF_SUCCESS) {
		return err_code;
	}

	// If enabled, call the app
	if (g_AUTO_ENABLE_MICROAPP_ON_BOOT || storage.isEnabled()) {
		LOGi("Enable microapp");
		_enabled = true;
		// Actually call app
		protocol.callApp();
	} else {
		LOGi("Microapp is not enabled");
	}
	return err_code;
}

/*
 * Currently the implementation sends a return message several times. The sender has to receive the return message
 * before the next message can be sent. Especially when sending a large binary chunk (like a microapp), it is 
 * useful to send a few semi-duplicates so that messages are not missed. 
 */
void Microapp::tick() {
	MicroappProtocol & protocol = MicroappProtocol::getInstance();
	protocol.callSetupAndLoop();
}

cs_ret_code_t Microapp::handleGetInfo(cs_result_t& result) {
	if (result.buf.len < sizeof(microapp_info_t)) {
		return ERR_BUFFER_TOO_SMALL;
	}
	microapp_info_t* info = reinterpret_cast<microapp_info_t*>(result.buf.data);

	info->protocol = MICROAPP_PROTOCOL;
	info->maxApps = MAX_MICROAPPS;
	info->maxAppSize = MICROAPP_MAX_SIZE;
	info->maxChunkSize = MICROAPP_UPLOAD_MAX_CHUNK_SIZE;
	info->maxRamUsage = MICROAPP_MAX_RAM;
	info->sdkVersion.major = MICROAPP_SDK_MAJOR;
	info->sdkVersion.minor = MICROAPP_SDK_MINOR;
//	info.appsStatus[0].
	return ERR_SUCCESS;
}

cs_ret_code_t Microapp::handleUpload(microapp_upload_internal_t* packet) {
	if (packet->header.header.protocol != MICROAPP_PROTOCOL) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	MicroappStorage & storage = MicroappStorage::getInstance();
	// CAREFUL: This assumes the data stays in ram during the write.
	cs_ret_code_t retCode = storage.writeChunk(packet->header.header.index, packet->data.data, packet->data.len);

	// User should wait for the data to be written to flash before sending the next chunk.
	if (retCode == ERR_SUCCESS) {
		return ERR_WAIT_FOR_SUCCESS;
	}
	return retCode;
}

cs_ret_code_t Microapp::handleValidate(microapp_ctrl_header_t* packet) {
	if (packet->protocol != MICROAPP_PROTOCOL) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	MicroappStorage & storage = MicroappStorage::getInstance();
	return storage.validateApp(packet->index);
}

cs_ret_code_t Microapp::handleRemove(microapp_ctrl_header_t* packet) {
	if (packet->protocol != MICROAPP_PROTOCOL) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t Microapp::handleEnable(microapp_ctrl_header_t* packet) {
	if (packet->protocol != MICROAPP_PROTOCOL) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	MicroappStorage & storage = MicroappStorage::getInstance();
	return storage.enableApp(packet->index, true);
}

cs_ret_code_t Microapp::handleDisable(microapp_ctrl_header_t* packet) {
	if (packet->protocol != MICROAPP_PROTOCOL) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	MicroappStorage & storage = MicroappStorage::getInstance();
	return storage.enableApp(packet->index, false);
}


uint32_t Microapp::handlePacket(microapp_packet_header_t *packet_stub) {
	
	switch(packet_stub->opcode) {

		case CS_MICROAPP_OPCODE_ENABLE: {
			microapp_enable_packet_t* packet = reinterpret_cast<microapp_enable_packet_t*>(packet_stub);

			// write the offset to flash
			TYPIFY(STATE_MICROAPP) state_microapp;
			cs_state_data_t data(CS_TYPE::STATE_MICROAPP, packet->app_id,
					(uint8_t*)&state_microapp, sizeof(state_microapp));
			LOGi("Set of offset to 0x%x", packet->offset);
			State::getInstance().get(data);
			state_microapp.offset = packet->offset;
			State::getInstance().set(data);

			LOGi("Enable app");
			MicroappStorage & storage = MicroappStorage::getInstance();
			err_code = storage.enableApp(true);

			if (err_code == ERR_SUCCESS) {
				LOGi("Call app");
				MicroappProtocol & protocol = MicroappProtocol::getInstance();
				protocol.callApp();
			}
			break;
		}
		case CS_MICROAPP_OPCODE_DISABLE: {
			LOGi("Disable app");
			MicroappStorage & storage = MicroappStorage::getInstance();
			err_code = storage.enableApp(false);
			break;
		}
		case CS_MICROAPP_OPCODE_UPLOAD: {
			microapp_upload_packet_t* packet = reinterpret_cast<microapp_upload_packet_t*>(packet_stub);

			// Prepare notification packet.
			_prevMessage.app_id = packet->app_id;

			// Validate chunk in ram.
			LOGi("Validate chunk %i", packet->index);
			MicroappStorage & storage = MicroappStorage::getInstance();
			err_code = storage.validateChunk(packet->data, MICROAPP_CHUNK_SIZE, packet->checksum);
			if (err_code != ERR_SUCCESS) {
				break;
			}

			// Write chunk with fstorage to flash.
			err_code = storage.writeChunk(packet->index, packet->data, MICROAPP_CHUNK_SIZE);
			if (err_code != ERR_SUCCESS) {
				break;
			}
			LOGi("Successfully written to chunk");

			// For now tell the sending party to wait (storing to flash).
			if (err_code == ERR_SUCCESS) {
				_prevMessage.payload = packet->index;
				err_code = ERR_WAIT_FOR_SUCCESS;
			}
			break;
		}
		case CS_MICROAPP_OPCODE_VALIDATE: {
			microapp_validate_packet_t* packet = reinterpret_cast<microapp_validate_packet_t*>(packet_stub);
			LOGi("Store meta data (checksum, etc.)");
			MicroappStorage & storage = MicroappStorage::getInstance();
			storage.storeAppMetadata(packet->app_id, packet->checksum, packet->size);

			// Assumes that storage of meta data gets actually through at one point
			LOGi("Validate app");
			err_code = storage.validateApp();
			if (err_code != ERR_SUCCESS) {
				break;
			}

			// Set app to valid
			LOGi("Set app valid");
			storage.setAppValid();

			break;
		}
		default: {
			LOGw("Microapp: Unknown opcode");
			err_code = ERR_NOT_IMPLEMENTED;
			break;
		}
	}
	return err_code;
}

/*
 * Handle incoming events from other modules (mainly the CommandController).
 */
void Microapp::handleEvent(event_t & evt) {

	// The err_code will be written to the event and returned to the caller over BLE.
	uint32_t err_code = ERR_EVENT_UNHANDLED;

	switch (evt.type) {
		case CS_TYPE::CMD_MICROAPP_GET_INFO: {
			evt.result.returnCode = handleGetInfo(evt.result);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_UPLOAD: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_UPLOAD, evt.data);
			evt.result.returnCode = handleUpload(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_VALIDATE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_VALIDATE, evt.data);
			evt.result.returnCode = handleValidate(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_REMOVE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_REMOVE, evt.data);
			evt.result.returnCode = handleRemove(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_ENABLE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_ENABLE, evt.data);
			evt.result.returnCode = handleEnable(packet);
			break;
		}
		case CS_TYPE::CMD_MICROAPP_DISABLE: {
			auto packet = CS_TYPE_CAST(CMD_MICROAPP_DISABLE, evt.data);
			evt.result.returnCode = handleDisable(packet);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			tick();
			break;
		}
		default: {
			// ignore other events
		}
	}
}
