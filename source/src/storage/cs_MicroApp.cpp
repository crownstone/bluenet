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
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_MicroApp.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

void fs_evt_handler_sched(void *data, uint16_t size) {
	nrf_fstorage_evt_t * evt = (nrf_fstorage_evt_t*) data;
	MicroApp::getInstance().handleFileStorageEvent(evt);
}

static void fs_evt_handler(nrf_fstorage_evt_t * p_evt) {
	uint32_t retVal = app_sched_event_put(p_evt, sizeof(*p_evt), fs_evt_handler_sched);
	APP_ERROR_CHECK(retVal);
}

#define MICRO_APP_PAGES      4

/**
 * Storage, 4 pages reserved. From 68000 up to 6C000.
 */
NRF_FSTORAGE_DEF(nrf_fstorage_t micro_app_storage) =
{
	.evt_handler    = fs_evt_handler,
	.start_addr     = 0x68000,
	.end_addr       = 0x68000 + (0x1000*MICRO_APP_PAGES) - 1,
};

MicroApp::MicroApp(): EventListener() {
}

int MicroApp::init() {
	uint32_t err_code;
	err_code = nrf_fstorage_init(&micro_app_storage, &nrf_fstorage_sd, NULL);
	return err_code;
}

int MicroApp::erasePages() {
	uint32_t err_code;
	err_code = nrf_fstorage_erase(&micro_app_storage, micro_app_storage.start_addr, MICRO_APP_PAGES, NULL);
	return err_code;
}

/**
 * We assume that the checksum of the particular chunk is already performed. It would be a waste to send an event with
 * incorrect checksum. If other sources for microapp code (e.g. UART) are added, the checksum should also be checked
 * early in the process. We also assume that all data buffers are of size MICROAPP_CHUNK_SIZE except for the last
 * packet.
 */
int MicroApp::writeChunk(uint8_t index, uint8_t *data, uint8_t size) {
	uint32_t err_code;
	uint32_t start = micro_app_storage.start_addr + MICROAPP_CHUNK_SIZE * index;
	if ((start + size) > micro_app_storage.end_addr) {
		LOGw("Microapp binary too large. Chunk not written");
		return ERR_NO_SPACE;
	}
	err_code = nrf_fstorage_write(&micro_app_storage, start, data, size, NULL);
	return err_code;
}

/**
 * For now only allow one app.
 */
void MicroApp::storeAppMetadata(uint8_t id, uint16_t checksum, uint16_t size) {
	TYPIFY(STATE_MICROAPP) state_microapp;
	state_microapp.start_addr = micro_app_storage.start_addr;
	state_microapp.size = size;
	state_microapp.checksum = checksum;
	state_microapp.id = id;
	state_microapp.validation = static_cast<uint8_t>(MICROAPP_VALIDATION::NONE);
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().set(data);
}

uint8_t MicroApp::validateApp() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	
	// temporary buffer (large one!)
	uint8_t buf[MICROAPP_CHUNK_SIZE];

	// read from flash with fstorage, calculate checksum
	uint8_t count = (state_microapp.size - 1) / MICROAPP_CHUNK_SIZE;
	uint16_t remain = state_microapp.size - (count * MICROAPP_CHUNK_SIZE);
	uint16_t checksum = 0;
	uint32_t addr = micro_app_storage.start_addr;
	for (int i = 0; i < count; ++i) { 
		nrf_fstorage_read(&micro_app_storage, addr, &buf, MICROAPP_CHUNK_SIZE);
		checksum = Fletcher(buf, MICROAPP_CHUNK_SIZE, checksum);
		addr += MICROAPP_CHUNK_SIZE;
	}
	if (remain) {
		nrf_fstorage_read(&micro_app_storage, addr, &buf, remain);
		checksum = Fletcher(buf, remain, checksum);
	}
	
	// compare checksum
	if (state_microapp.checksum == checksum) {
		LOGi("Yes, micro app %i has the same checksum", state_microapp.id);
	} else {
		LOGw("Yes, micro app %i has checksum %x, but calculation shows %x", state_microapp.id,
				state_microapp.checksum, checksum);
		return ERR_INVALID_MESSAGE; // better to have ERR_INVALID or ERR_CHECKSUM_INCORRECT
	}
	
	// write validation = VALIDATION_CHECKSUM to fds record
	state_microapp.validation = static_cast<uint8_t>(MICROAPP_VALIDATION::CHECKSUM);
	State::getInstance().set(data);
	return EXIT_SUCCESS;
}

void MicroApp::handleEvent(event_t & evt) {
	switch (evt.type) {
	case CS_TYPE::CMD_MICROAPP_UPLOAD: {
		microapp_upload_packet_t* packet = reinterpret_cast<TYPIFY(CMD_MICROAPP_UPLOAD)*>(evt.data);
		uint32_t err_code = writeChunk(packet->index, packet->data, packet->size);
		if (err_code == ERR_SUCCESS) {
			LOGi("Successfully written to chunk");
			if (packet->index == packet->count - 1) {
				// for now accept only apps with id == 0
				assert(packet->app_id == 0, "Only app id == 0 allowed");

				// write app meta info to fds
				uint16_t size = packet->index * MICROAPP_CHUNK_SIZE + packet->size;
				storeAppMetadata(packet->app_id, packet->app_checksum, size);

				// validate app
				validateApp();
			}
		}
		break;
	}
	default: {
		// ignore other events
	}
	}
}

void MicroApp::handleFileStorageEvent(nrf_fstorage_evt_t *evt) {
	if (evt->result != NRF_SUCCESS) {
		LOGe("Flash error");
		return;
	} else {
		LOGi("Flash event successful");
	}

	switch (evt->id) {
	case NRF_FSTORAGE_EVT_WRITE_RESULT: {
		LOGi("Flash written");
		TYPIFY(EVT_MICROAPP) data;
		data = NRF_FSTORAGE_EVT_WRITE_RESULT;
		event_t event(CS_TYPE::EVT_MICROAPP, &data, sizeof(data));
		event.dispatch();
		break;
	}
	case NRF_FSTORAGE_EVT_ERASE_RESULT: {
		LOGi("Flash erased");
		break;
	}
	default:
		break;
	}
}
