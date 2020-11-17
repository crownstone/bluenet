/** Store "micro" apps in flash.
 *
 * Write a file to flash. This class listens to events and stores them in flash. The location where it is stored in 
 * flash is separate from the bluenet binary. It can be seen as a DFU process where bluenet functions as a bootloader
 * for yet other binaries that we call here micro apps.
 *
 * A typical micro app can be a small binary compiled with Arduino conventions for the Crownstone architecture.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "nrf_fstorage_sd.h"

#include <algorithm>
#include <ble/cs_UUID.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <microapp/cs_MicroApp.h>
#include <microapp/cs_MicroAppProtocol.h>
#include <microapp/cs_MicroAppStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

MicroApp::MicroApp(): EventListener() {
	
	EventDispatcher::getInstance().addListener(this);

	_prevMessage.protocol = 0;
	_prevMessage.app_id = 0;
	_prevMessage.payload = 0;
	_prevMessage.repeat = 0;

	_enabled = false;
	_debug = true;

	
}

uint16_t MicroApp::init() {
	uint32_t err_code;

	MicroAppStorage & storage = MicroAppStorage::getInstance();
	storage.init();

	// Set callback handler in IPC ram
	MicroAppProtocol & protocol = MicroAppProtocol::getInstance();
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
	if (storage.isEnabled()) {
		LOGi("Enable microapp");
		_enabled = true;
		// Actually call app
		protocol.callApp();
	} else {
		LOGi("Microapp is not enabled");
	}
	return err_code;
}

void MicroApp::tick() {
	// decrement repeat counter up to zero
	if (_prevMessage.repeat > 0) {
		_prevMessage.repeat--;
	}
	// only sent when repeat counter is non-zero
	if (_prevMessage.repeat > 0) {
		event_t event(CS_TYPE::EVT_MICROAPP, &_prevMessage, sizeof(_prevMessage));
		event.dispatch();
	}
					
	MicroAppProtocol & protocol = MicroAppProtocol::getInstance();
	protocol.callSetupAndLoop();

}

uint32_t MicroApp::handlePacket(microapp_packet_header_t *packet_stub) {
	
	uint32_t err_code = ERR_EVENT_UNHANDLED;
	
	// For now accept only apps with id == 0.
	if (packet_stub->app_id != 0) {
		err_code = ERR_NOT_IMPLEMENTED;
		return err_code;
	}

	switch(packet_stub->opcode) {
	case CS_MICROAPP_OPCODE_REQUEST: {
		microapp_request_packet_t* packet = reinterpret_cast<microapp_request_packet_t*>(packet_stub);
		// Check if app size is not too large.
		MicroAppStorage & storage = MicroAppStorage::getInstance();
		err_code = storage.checkAppSize(packet->size);
		if (err_code != ERR_SUCCESS) {
			break;
		}

		if (packet->chunk_size != MICROAPP_CHUNK_SIZE) {
			err_code = ERR_WRONG_STATE;
			LOGi("Chunk size %i (should be %i)", packet->chunk_size, MICROAPP_CHUNK_SIZE);
			_prevMessage.payload = MICROAPP_CHUNK_SIZE;
			break;
		}

		if (packet->size > packet->count * packet->chunk_size) {
			err_code = ERR_WRONG_PARAMETER;
			_prevMessage.payload = packet->count * packet->chunk_size;
			break;
		}

		// We can add here a method to check if the memory is clear... However, this is quite an intensive
		// read of flash memory. So, it is probably smart to do this with another opcode.
		break;
	}
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
		MicroAppStorage & storage = MicroAppStorage::getInstance();
		err_code = storage.enableApp(true);

		if (err_code == ERR_SUCCESS) {
			LOGi("Call app");
			MicroAppProtocol & protocol = MicroAppProtocol::getInstance();
			protocol.callApp();
		}
		break;
	}
	case CS_MICROAPP_OPCODE_DISABLE: {
		LOGi("Disable app");
		MicroAppStorage & storage = MicroAppStorage::getInstance();
		err_code = storage.enableApp(false);
		break;
	}
	case CS_MICROAPP_OPCODE_UPLOAD: {
		microapp_upload_packet_t* packet = reinterpret_cast<microapp_upload_packet_t*>(packet_stub);
		
		// Prepare notification packet.
		_prevMessage.app_id = packet->app_id;

		// Validate chunk in ram.
		LOGi("Validate chunk %i", packet->index);
		MicroAppStorage & storage = MicroAppStorage::getInstance();
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
		MicroAppStorage & storage = MicroAppStorage::getInstance();
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

/**
 * Handle incoming events from other modules (mainly the CommandController).
 */
void MicroApp::handleEvent(event_t & evt) {

	// The err_code will be written to the event and returned to the caller over BLE.
	uint32_t err_code = ERR_EVENT_UNHANDLED;

	switch (evt.type) {
	case CS_TYPE::CMD_MICROAPP: {
		// Immediately stop previous notifications
		_prevMessage.repeat = 0;

		LOGi("MicroApp receives event");
		microapp_packet_header_t* packet = reinterpret_cast<microapp_packet_header_t*>(evt.data);

		// Handle packet
		err_code = handlePacket(packet);

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

	if (evt.type == CS_TYPE::CMD_MICROAPP) {
		LOGi("Return code %i", err_code);
		evt.result.returnCode = err_code;
	}
}
