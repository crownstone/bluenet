/*
 * Store "micro" apps in flash.
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
#include <microapp/cs_MicroappProtocol.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

void fs_evt_handler_sched(void *data, uint16_t size) {
	nrf_fstorage_evt_t * evt = (nrf_fstorage_evt_t*) data;
	MicroappStorage::getInstance().handleFileStorageEvent(evt);
}

static void fs_evt_handler(nrf_fstorage_evt_t * p_evt) {
	uint32_t retVal = app_sched_event_put(p_evt, sizeof(*p_evt), fs_evt_handler_sched);
	APP_ERROR_CHECK(retVal);
}

NRF_FSTORAGE_DEF(nrf_fstorage_t nrf_microapp_storage) =
{
	.evt_handler    = fs_evt_handler,
	.start_addr     = g_FLASH_MICROAPP_BASE,
	.end_addr       = g_FLASH_MICROAPP_BASE + (0x1000*(g_FLASH_MICROAPP_PAGES)) - 1,
};

MicroappStorage::MicroappStorage() { 

	_prevMessage.protocol = 0;
	_prevMessage.app_id = 0;
	_prevMessage.payload = 0;
	_prevMessage.repeat = 0;

	_enabled = false;
	_debug = true;
}

uint16_t MicroappStorage::init() {
	_buffer = new uint8_t[MICROAPP_CHUNK_SIZE];

	uint32_t err_code;
	err_code = nrf_fstorage_init(&nrf_microapp_storage, &nrf_fstorage_sd, NULL);
	switch(err_code) {
	case NRF_SUCCESS:
		LOGi("Sucessfully initialized from 0x%08X to 0x%08X", nrf_microapp_storage.start_addr, nrf_microapp_storage.end_addr);
		break;
	default:
		LOGw("Error code %i", err_code);
		break;
	}

	return err_code;
}

uint16_t MicroappStorage::erasePages() {
	uint32_t err_code;
	err_code = nrf_fstorage_erase(&nrf_microapp_storage, nrf_microapp_storage.start_addr, g_FLASH_MICROAPP_PAGES, NULL);
	return err_code;
}

uint16_t MicroappStorage::checkAppSize(uint16_t size) {
	uint32_t start = nrf_microapp_storage.start_addr;
	if ((start + size) > nrf_microapp_storage.end_addr) {
		LOGw("Microapp binary too large. Application can not be written");
		return ERR_NO_SPACE;
	}
	return ERR_SUCCESS;
}

/**
 * We assume that the checksum of the particular chunk is already performed. It would be a waste to send an event with
 * incorrect checksum. If other sources for microapp code (e.g. UART) are added, the checksum should also be checked
 * early in the process. We also assume that all data buffers are of size MICROAPP_CHUNK_SIZE. The last one should be
 * appended with 0xFF values to make it the right size.
 */
uint16_t MicroappStorage::writeChunk(uint8_t index, const uint8_t *data, uint16_t size) {
	uint32_t err_code;
	uint32_t start = nrf_microapp_storage.start_addr + (MICROAPP_CHUNK_SIZE * index);
	LOGi("Write chunk: %i at 0x%08X of size %i", index, start, size);
	LOGi("Start with data [%02X,%02X,%02X]", data[0], data[1], data[2]);

	// check chunk size
	if ((start + size) > nrf_microapp_storage.end_addr) {
		LOGw("Microapp binary too large. Chunk not written");
		return ERR_NO_SPACE;
	}
	// Make a copy of the data object
	memcpy(_buffer, data, size);

	err_code = nrf_fstorage_write(&nrf_microapp_storage, start, _buffer, size, NULL);
	switch(err_code) {
		case NRF_ERROR_NULL:
			LOGw("Error code %i, NRF_ERROR_NULL", err_code);
			break;
		case NRF_ERROR_INVALID_STATE:
			LOGw("Error code %i, NRF_ERROR_INVALID_STATE", err_code);
			break;
		case NRF_ERROR_INVALID_LENGTH:
			LOGw("Error code %i, NRF_ERROR_INVALID_LENGTH", err_code);
			LOGw("  start %i, data [%02X,%02X,%02X], size, %i", start, data[0], data[1], data[2], size);
			break;
		case NRF_ERROR_INVALID_ADDR:
			LOGw("Error code %i, NRF_ERROR_INVALID_ADDR", err_code);
			LOGw("  at 0x%08X (start at 0x%08X)", start, nrf_microapp_storage.start_addr);
			LOGw("  is 0x%08X word-aligned as well?", data);
			break;
		case NRF_ERROR_NO_MEM:
			LOGw("Error code %i, NRF_ERROR_NO_MEM", err_code);
			break;
		case NRF_SUCCESS:
			LOGi("Sucessfully written");
			break;
	}
	return err_code;
}

/**
 * For now only allow one app.
 */
void MicroappStorage::storeAppMetadata(uint8_t id, uint16_t checksum, uint16_t size) {
	LOGi("Store app %i, checksum %i, size %i, start address %x", id, checksum, size, nrf_microapp_storage.start_addr);
	TYPIFY(STATE_MICROAPP) state_microapp;
	state_microapp.start_addr = nrf_microapp_storage.start_addr;
	state_microapp.size = size;
	state_microapp.checksum = checksum;
	state_microapp.id = id;
	state_microapp.validation = static_cast<uint8_t>(CS_MICROAPP_VALIDATION_NONE);
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	uint32_t err_code = State::getInstance().set(data);

	if (err_code != ERR_SUCCESS) {
		LOGi("Error: remove state microapp");
		Storage::getInstance().remove(CS_TYPE::STATE_MICROAPP);
	}
}

/**
 * Validate a chunk of data given a checksum to compare with.
 */
uint16_t MicroappStorage::validateChunk(const uint8_t * const data, uint16_t size, uint16_t compare) {
	uint32_t checksum = 0;
	checksum = Fletcher(data, size, checksum);
	LOGi("Chunk checksum %04X (versus %04X)", (uint16_t)checksum, compare);
	if ((uint16_t)checksum == compare) {
		return ERR_SUCCESS;
	} else {
		return ERR_INVALID_MESSAGE;
	}
}

/**
 * Get configuration from the header in the app.
 */
void MicroappStorage::getHeaderApp(microapp_header_t *header) {

	LOGd("Get app header");
	const uint32_t addr = nrf_microapp_storage.start_addr;
	const uint8_t size = sizeof(microapp_header_t);
	uint8_t buf[size];

	nrf_fstorage_read(&nrf_microapp_storage, addr, &buf, size);
	header->offset   = (uint32_t)((buf[1] << 8) + (buf[0]));
	header->size     = (uint32_t)((buf[5] << 8) + (buf[4]));
	header->checksum = (uint32_t)((buf[9] << 8) + (buf[8]));

	LOGd("Offset: %i", header->offset);
	LOGd("Size: %i", header->size);
	LOGd("Checksum: %x", header->checksum);
}

/**
 * This function validates the app in flash. Only use it when all chunks, also the last chunk has been written 
 * successfully.
 *
 * The checksum is calculated iteratively, by using the chunk size. For the last chunk we will only use the part 
 * up to the total size of the binary. By doing it iteratively we can keep the local buffer relatively small.
 */
uint16_t MicroappStorage::validateApp() {
	LOGd("Validate app");
	uint16_t ret_code;

	microapp_header_t header;
	getHeaderApp(&header);
	
	if (header.size > 0x1000*(g_FLASH_MICROAPP_PAGES)) {
		LOGi("Microapp size, %i, too large. Do not load", header.size);
		return ERR_WRONG_STATE;
	}
    if (header.offset > 0x1000*(g_FLASH_MICROAPP_PAGES)) {
		LOGi("Microapp offset, %i, too large. Do not load", header.offset);
		return ERR_WRONG_STATE;
    }
	LOGd("Microapp size field is: %x", header.size);

	// get the state fields for the microapp and store results
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);

	state_microapp.start_addr = nrf_microapp_storage.start_addr;
	state_microapp.offset = header.offset;
	state_microapp.size = header.size;
	state_microapp.checksum = header.checksum;
	
	LOGd("Write microapp config");
	State::getInstance().set(data);
	
	// read from flash with fstorage, calculate checksum
	LOGd("Calculate checksum, should become: %x", header.checksum);
	uint8_t count = state_microapp.size / MICROAPP_CHUNK_SIZE;
	uint16_t remain = state_microapp.size - (count * MICROAPP_CHUNK_SIZE);
	uint32_t checksum_iterative = 0;

	// loop and limit required data to MICROAPP_CHUNK_SIZE
	uint8_t buf[MICROAPP_CHUNK_SIZE];
	uint32_t addr = nrf_microapp_storage.start_addr;

	// actually we now fletcher returns 0 for all zeros, but already in place if we use something else
	memset(&buf, 0, sizeof(header));
	uint8_t header_count = sizeof(header) / MICROAPP_CHUNK_SIZE;
	uint16_t header_remain = sizeof(header) - header_count;
	for (int i = 0; i < header_count; ++i) {
		checksum_iterative = Fletcher(buf, MICROAPP_CHUNK_SIZE, checksum_iterative);
		addr += MICROAPP_CHUNK_SIZE;
	}
	if (header_remain) {
		checksum_iterative = Fletcher(buf, header_remain, checksum_iterative);
		addr += header_remain;
	}

	for (int i = 0; i < count; ++i) {
		ret_code = nrf_fstorage_read(&nrf_microapp_storage, addr, &buf, MICROAPP_CHUNK_SIZE);
		if (ret_code != ERR_SUCCESS) {
			LOGw("Error with reading with fstorage: %i", ret_code);
			return ERR_INVALID_MESSAGE;
		}
		checksum_iterative = Fletcher(buf, MICROAPP_CHUNK_SIZE, checksum_iterative);
		addr += MICROAPP_CHUNK_SIZE;
	}
	// last chunk of alternative size (smaller than MICROAPP_CHUNK_SIZE)
	if (remain) {
		nrf_fstorage_read(&nrf_microapp_storage, addr, &buf, remain);
		checksum_iterative = Fletcher(buf, remain, checksum_iterative);
		addr += remain;
	}
	// checksum is truncated to 16 bits
	uint16_t checksum = (uint16_t)checksum_iterative;
	LOGd("Calculated checksum: %x", checksum);
	// compare checksum
	if (state_microapp.checksum != checksum) {
		LOGw("Microapp %i has checksum 0x%04X, but calculation shows 0x%04X", state_microapp.id,
				state_microapp.checksum, checksum);
		return ERR_INVALID_MESSAGE;
	}
	
	LOGi("Yes, microapp %i has proper checksum", state_microapp.id);
	return ERR_SUCCESS;
}

/**
 * Set the app to be "valid". This is normally done after the checksum has been checked and considered correct.
 * This information is written to flash.
 */
void MicroappStorage::setAppValid() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	
	// write validation = VALIDATION_CHECKSUM to fds record
	state_microapp.validation = static_cast<uint8_t>(CS_MICROAPP_VALIDATION_CHECKSUM);
	State::getInstance().set(data);
}

/**
 * Get if the app is actually valid. If not available from memory it will be obtained from flash.
 */
bool MicroappStorage::isAppValid() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	return (state_microapp.validation == static_cast<uint8_t>(CS_MICROAPP_VALIDATION_CHECKSUM));
}

/**
 * After an app has been considered "valid" it is possible to enable it. It is also possible to disable an app
 * afterwards (without invalidating it). The app can be enabled later on again.
 */
uint16_t MicroappStorage::enableApp(bool flag) {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);

	if (state_microapp.validation >= static_cast<uint8_t>(CS_MICROAPP_VALIDATION_CHECKSUM)) {
		if (flag) {
			state_microapp.validation = static_cast<uint8_t>(CS_MICROAPP_VALIDATION_ENABLED);
			_enabled = true;
		} else {
			state_microapp.validation = static_cast<uint8_t>(CS_MICROAPP_VALIDATION_DISABLED);
			_enabled = false;
		}
	} else {
		// Not allowed to enable/disable app that did not pass checksum valid state.
		return ERR_INVALID_MESSAGE;
	}
	State::getInstance().set(data);
	return ERR_SUCCESS;
}

/**
 * Check if an app is enabled.
 */
bool MicroappStorage::isEnabled() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	return (state_microapp.validation == static_cast<uint8_t>(CS_MICROAPP_VALIDATION_ENABLED));
}

/**
 * Return result of fstorage operation to sender. We only send this event after fstorage returns to pace the 
 * incoming messages. In this way it is also possible to resend a particular chunk.
 */
void MicroappStorage::handleFileStorageEvent(nrf_fstorage_evt_t *evt) {
	uint16_t error;
	if (evt->result != NRF_SUCCESS) {
		LOGe("Flash error");
		error = ERR_UNSPECIFIED;
	} else {
		LOGi("Flash event successful");
		error = ERR_SUCCESS;
	}

	switch (evt->id) {
		case NRF_FSTORAGE_EVT_WRITE_RESULT: {
			LOGi("Flash written");
			_prevMessage.repeat = number_of_notifications;
			_prevMessage.error = error;
			event_t event(CS_TYPE::EVT_MICROAPP, &_prevMessage, sizeof(_prevMessage));
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

