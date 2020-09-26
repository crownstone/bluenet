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

#define MICRO_APP_PAGES      2

/**
 * Storage, 4 pages reserved. From 68000 up to 6C000.
 */
NRF_FSTORAGE_DEF(nrf_fstorage_t micro_app_storage) =
{
	.evt_handler    = fs_evt_handler,
	.start_addr     = 0x68000,
	.end_addr       = 0x68000 + (0x1000*(MICRO_APP_PAGES)) - 1,
};

MicroApp::MicroApp(): EventListener() {
	
	EventDispatcher::getInstance().addListener(this);

	_prevMessage.protocol = 0;
	_prevMessage.app_id = 0;
	_prevMessage.payload = 0;
	_prevMessage.repeat = 0;

	_setup = 0;
	_loop = 0;
	_enabled = false;
	_booted = false;
	_debug = true;
}

uint16_t MicroApp::init() {
	_buffer = new uint8_t[MICROAPP_CHUNK_SIZE];

	uint32_t err_code;
	err_code = nrf_fstorage_init(&micro_app_storage, &nrf_fstorage_sd, NULL);
	switch(err_code) {
	case NRF_SUCCESS:
		LOGi("Sucessfully initialized from 0x%08X to 0x%08X", micro_app_storage.start_addr, micro_app_storage.end_addr);
		break;
	default:
		LOGw("Error code %i", err_code);
		break;
	}

	if (err_code != NRF_SUCCESS) {
		return err_code;
	}

	// Actually, first check if there's anything there?

	// Check for valid app on boot 
	err_code = validateApp();
	if (err_code == ERR_SUCCESS) {
		LOGi("Set app valid");
		setAppValid();
	} else {
		LOGi("Checksum error");
	}

	if (err_code != NRF_SUCCESS) {
		return err_code;
	}

	// If enabled, call the app
	if (isEnabled()) {
		LOGi("Enable microapp");
		_enabled = true;
		// Actually call app
		callApp();
	} else {
		LOGi("Microapp is not enabled");
	}
	return err_code;
}

uint16_t MicroApp::initMemory() {
	// We allow an area of 0x2000B000 and then two pages for RAM. For now let us clear it to 0
	// This is actually incorrect (we should skip) and it probably can be done at once as well
	for (int i = 0; i < 1024 * 2; ++i) {
		uint32_t *const val = (uint32_t *)(uintptr_t)(0x2000B000 + i);
		*val = 0;
	}

	// The above is fine for .bss (which is uninitialized) but not for .data. It needs to be copied
	// to the right addresses.
	return ERR_SUCCESS;
}

uint16_t MicroApp::erasePages() {
	uint32_t err_code;
	err_code = nrf_fstorage_erase(&micro_app_storage, micro_app_storage.start_addr, MICRO_APP_PAGES, NULL);
	return err_code;
}

uint16_t MicroApp::checkAppSize(uint16_t size) {
	uint32_t start = micro_app_storage.start_addr;
	if ((start + size) > micro_app_storage.end_addr) {
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
uint16_t MicroApp::writeChunk(uint8_t index, const uint8_t *data, uint16_t size) {
	uint32_t err_code;
	uint32_t start = micro_app_storage.start_addr + (MICROAPP_CHUNK_SIZE * index);
	LOGi("Write chunk: %i at 0x%08X of size %i", index, start, size);
	LOGi("Start with data [%02X,%02X,%02X]", data[0], data[1], data[2]);

	// check chunk size
	if ((start + size) > micro_app_storage.end_addr) {
		LOGw("Microapp binary too large. Chunk not written");
		return ERR_NO_SPACE;
	}
	// Make a copy of the data object
	memcpy(_buffer, data, size);

	err_code = nrf_fstorage_write(&micro_app_storage, start, _buffer, size, NULL);
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
		LOGw("  at 0x%08X (start at 0x%08X)", start, micro_app_storage.start_addr);
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
void MicroApp::storeAppMetadata(uint8_t id, uint16_t checksum, uint16_t size) {
	TYPIFY(STATE_MICROAPP) state_microapp;
	state_microapp.start_addr = micro_app_storage.start_addr;
	state_microapp.size = size;
	state_microapp.checksum = checksum;
	state_microapp.id = id;
	state_microapp.validation = static_cast<uint8_t>(CS_MICROAPP_VALIDATION_NONE);
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().set(data);
}

uint16_t MicroApp::validateChunk(const uint8_t * const data, uint16_t size, uint16_t compare) {
	uint32_t checksum = 0;
	checksum = Fletcher(data, size, checksum);
	LOGi("Chunk checksum %04X (versus %04X)", (uint16_t)checksum, compare);
	if ((uint16_t)checksum == compare) {
		return ERR_SUCCESS;
	} else {
		return ERR_INVALID_MESSAGE;
	}
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

	static uint16_t counter = 0;
	if (_enabled && _booted) {
		
		if (!_loaded) {
			uint16_t ret_code = interpretRamdata();
			if (ret_code == ERR_SUCCESS) {
				_loaded = true;
			} else {
				LOGw("Disable microapp. After boot not the right info available");
				// Apparently, something went wrong?
				_booted = false;
				_enabled = false;
			}
		}

		if (_loaded) {
			if (counter == 0) {
				LOGi("Call setup");
				void (*setup_func)() = (void (*)()) _setup;
				setup_func();
				LOGi("Setup done");
			}
			counter++;
			// Call loop every 10 ticks
			if (counter % 10 == 0) {
				LOGi("Call loop");
				int (*loop_func)() = (int (*)()) _loop;
				int result = loop_func();
				LOGi("Loop 0x%x", result);
			}
			// Loop around, but do not hit 0 again
			if (counter == 0) {
				counter = 1;
			}
		}
	}
}

/**
 * This function validates the app in flash. It can only be used when the state is written and all flash operations
 * have finished. Only use it when all chunks, also the last chunk has been written successfully.
 *
 * The checksum is calculated iteratively, by using the chunk size. For the last chunk we will only use the part 
 * up to the total size of the binary. By doing it iteratively we can keep the local buffer relatively small.
 */
uint16_t MicroApp::validateApp() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	
	uint8_t buf[MICROAPP_CHUNK_SIZE];

	// read from flash with fstorage, calculate checksum
	uint8_t count = state_microapp.size / MICROAPP_CHUNK_SIZE;
	uint16_t remain = state_microapp.size - (count * MICROAPP_CHUNK_SIZE);
	uint32_t checksum_iterative = 0;
	uint32_t addr = micro_app_storage.start_addr;
	for (int i = 0; i < count; ++i) {
		nrf_fstorage_read(&micro_app_storage, addr, &buf, MICROAPP_CHUNK_SIZE);
		checksum_iterative = Fletcher(buf, MICROAPP_CHUNK_SIZE, checksum_iterative);
		addr += MICROAPP_CHUNK_SIZE;
	}
	// last chunk of alternative size
	if (remain) {
		nrf_fstorage_read(&micro_app_storage, addr, &buf, remain);
		checksum_iterative = Fletcher(buf, remain, checksum_iterative);
	}
	uint16_t checksum = (uint16_t)checksum_iterative;
	// compare checksum
	if (state_microapp.checksum != checksum) {
		LOGw("Micro app %i has checksum 0x%04X, but calculation shows 0x%04X", state_microapp.id,
				state_microapp.checksum, checksum);
		return ERR_INVALID_MESSAGE;
	}
	
	LOGi("Yes, micro app %i has proper checksum", state_microapp.id);
	return ERR_SUCCESS;
}

void MicroApp::setAppValid() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	
	// write validation = VALIDATION_CHECKSUM to fds record
	state_microapp.validation = static_cast<uint8_t>(CS_MICROAPP_VALIDATION_CHECKSUM);
	State::getInstance().set(data);
}

bool MicroApp::isAppValid() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	return (state_microapp.validation == static_cast<uint8_t>(CS_MICROAPP_VALIDATION_CHECKSUM));
}

uint16_t MicroApp::enableApp(bool flag) {
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

bool MicroApp::isEnabled() {
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);
	return (state_microapp.validation == static_cast<uint8_t>(CS_MICROAPP_VALIDATION_ENABLED));
}

/**
 * Call the app, boot it.
 */
void MicroApp::callApp() {
	static bool thumb_mode = true;

	if (!isEnabled()) {
		LOGi("Microapp: app not enabled.")
		return;
	}
	
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);

	if (state_microapp.start_addr == 0x00) {
		LOGi("Module can't be run. Start address 0?");
		_booted = false;
		return;
	}

	initMemory();

	uintptr_t address = state_microapp.start_addr + state_microapp.offset;
	LOGi("Microapp: start at 0x%04x", address)

	if (thumb_mode) address += 1;
	LOGi("Check main code at %p", address);
	char *arr = (char*)address;
	if (arr[0] != 0xFF) {
		void (*microapp_main)() = (void (*)()) address;
		LOGi("Call function in module: %p", microapp_main);
		(*microapp_main)();
		LOGi("Module run.");
	}
	_booted = true;
}

uint16_t MicroApp::interpretRamdata() {
	uint8_t buf[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	for (int i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		buf[i] = 0;
	}
	uint8_t rd_size = 0;
	uint8_t ret_code = getRamData(IPC_INDEX_MICROAPP, buf, BLUENET_IPC_RAM_DATA_ITEM_SIZE, &rd_size);
	
	bluenet_ipc_ram_data_item_t *ramStr = getRamStruct(IPC_INDEX_MICROAPP);
	if (ramStr != NULL) {
		LOGi("Get microapp info from address: %p", ramStr);
	}

	if (_debug) {
		LOGi("Return code: %i", ret_code);
		LOGi("Return length: %i", rd_size);
		LOGi("  protocol: %02x", buf[0]);
		LOGi("  setup():  %02x %02x %02x %02x", buf[1], buf[2], buf[3], buf[4]);
		LOGi("  loop():   %02x %02x %02x %02x", buf[5], buf[6], buf[7], buf[8]);
	}

	if (ret_code == IPC_RET_SUCCESS) {
		LOGi("Found RAM for MicroApp");
		LOGi("  protocol: %02x", buf[0]);
		LOGi("  setup():  %02x %02x %02x %02x", buf[1], buf[2], buf[3], buf[4]);
		LOGi("  loop():   %02x %02x %02x %02x", buf[5], buf[6], buf[7], buf[8]);
	
		uint8_t protocol = buf[0];
		if (protocol == 0) {
			_setup = 0, _loop =0;
			uint8_t offset = 1;
			for (int i = 0; i < 4; ++i) {
				_setup = _setup | ( (uintptr_t)(buf[i+offset]) << (i*8));
			}
			offset = 5;
			for (int i = 0; i < 4; ++i) {
				_loop = _loop | ( (uintptr_t)(buf[i+offset]) << (i*8));
			}
		}

		LOGi("Found setup at %p", _setup);
		LOGi("Found loop at %p", _loop);
		if (_loop && _setup) {
			return ERR_SUCCESS;
		}
	} else {
		LOGi("Nothing found in RAM");
		return ERR_NOT_FOUND;
	}
	return ERR_UNSPECIFIED;
}

/**
 * Return result of fstorage operation to sender. We only send this event after fstorage returns to pace the 
 * incoming messages. In this way it is also possible to resend a particular chunk.
 */
void MicroApp::handleFileStorageEvent(nrf_fstorage_evt_t *evt) {
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
		err_code = checkAppSize(packet->size);
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
		err_code = enableApp(true);

		if (err_code == ERR_SUCCESS) {
			LOGi("Call app");
			callApp();
		}
		break;
	}
	case CS_MICROAPP_OPCODE_DISABLE: {
		LOGi("Disable app");
		err_code = enableApp(false);
		break;
	}
	case CS_MICROAPP_OPCODE_UPLOAD: {
		microapp_upload_packet_t* packet = reinterpret_cast<microapp_upload_packet_t*>(packet_stub);
		
		// Prepare notification packet.
		_prevMessage.app_id = packet->app_id;

		// Validate chunk in ram.
		LOGi("Validate chunk %i", packet->index);
		err_code = validateChunk(packet->data, MICROAPP_CHUNK_SIZE, packet->checksum);
		if (err_code != ERR_SUCCESS) {
			break;
		}
		
		// Write chunk with fstorage to flash.
		err_code = writeChunk(packet->index, packet->data, MICROAPP_CHUNK_SIZE);
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
		storeAppMetadata(packet->app_id, packet->checksum, packet->size);

		// Assumes that storage of meta data gets actually through at one point
		LOGi("Validate app");
		err_code = validateApp();
		if (err_code != ERR_SUCCESS) {
			break;
		}
		
		// Set app to valid
		LOGi("Set app valid");
		setAppValid();
		
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
