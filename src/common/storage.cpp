/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

/**
 * For more information see:
 * http://developer.nordicsemi.com/nRF51_SDK/doc/7.0.1/s110/html/a00763.html#ga0a57b964c8945eaca2d267835ef6688c
 */

#include <common/storage.h>
#include <util/ble_error.h>
#include <drivers/serial.h>

#include <BluetoothLE.h> // TODO: now for BLE_CALL, should be moved to other unit

extern "C" {
#include <app_scheduler.h>

//static pstorage_handle_t handle;
//static pstorage_handle_t block_handle;

static void pstorage_callback_handler(pstorage_handle_t * handle, uint8_t op_code, uint32_t result, uint8_t * p_data, 
		uint32_t data_len) {
	// we might want to check if things are actually stored, by using this callback	
	if (result != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", result, result);
		APP_ERROR_CHECK(result);
		
		if (op_code == PSTORAGE_LOAD_OP_CODE) {
			LOGd("Error with loading data");
		}
	} else {
		log(INFO, "Opcode %i executed (no error)", op_code);
	}
}

} // extern "C"

Storage::Storage() {
}

Storage::~Storage() {
}

/**
 * We allocate a single block of size "size". Biggest allocated size is 640 bytes.
 */
void Storage::init(int size) {
	log(INFO, "Create persistent storage (FLASH) of %i bytes", size);
	_size = size;

	// call once before using any other API calls of the persistent storage module
	BLE_CALL(pstorage_init, ());

	// set parameter
	pstorage_module_param_t param;
	param.block_size = size;
	param.block_count = 1;
	param.cb = pstorage_callback_handler;

	// register
	BLE_CALL ( pstorage_register, (&param, &handle) );

	// get block id
	BLE_CALL ( pstorage_block_identifier_get,(&handle, 0, &block_handle) );
}

/**
 * We have to clear an entire block before we can write a value to one of the fields!
 */
void Storage::clear() {
	log(WARNING, "Nordic bug: clear entire block before writing to it");
	BLE_CALL (pstorage_clear, (&block_handle, _size) );
}

// Get byte at location "index" 
void Storage::getUint8(int index, uint8_t *item) {
	static uint8_t wg8[4];
	BLE_CALL (pstorage_load, (wg8, &block_handle, 4, index)); 
	*item = wg8[0];
}

// Store byte
void Storage::setUint8(int index, const uint8_t item) {
	clear();
	static uint8_t ws8[4];
	ws8[0] = item;
	ws8[1] = 0;
	ws8[2] = 0;
	ws8[3] = 0;
	BLE_CALL (pstorage_store, (&block_handle, ws8, 4, index) );
}

//#define PRINT_ITEMS
//#define PRINT_PENDING

// Get 16-bit integer
void Storage::getUint16(int index, uint16_t *item) {
	static uint8_t wg16[4];
	BLE_CALL (pstorage_load, (wg16, &block_handle, 4, index) ); 
#ifdef PRINT_ITEMS
	LOGd("Load item[0]: %d", wg16[0]);
	LOGd("Load item[1]: %d", wg16[1]);
	LOGd("Load item[2]: %d", wg16[2]);
	LOGd("Load item[3]: %d", wg16[3]);
#endif
//	*item = 0;
	*item = ((uint16_t)wg16[1]) << 8;
       	*item |= wg16[0];
#ifdef PRINT_PENDINSG
	uint32_t count;
	BLE_CALL ( pstorage_access_status_get, (&count) );
	log(INFO, "Number of pending operations: %i", count);
#endif
}

// Set 16-bit integer
void Storage::setUint16(int index, const uint16_t item) {
	// TODO: we now clear the entire block!
	clear();

	static uint8_t ws16[4];
	ws16[0] = item;
	ws16[1] = item >> 8;
	ws16[2] = 0;
	ws16[3] = 0;
#ifdef PRINT_ITEMS
	LOGd("Store item[0]: %d", ws16[0]);
	LOGd("Store item[1]: %d", ws16[1]);
	LOGd("Store item[2]: %d", ws16[2]);
	LOGd("Store item[3]: %d", ws16[3]);
#endif
	BLE_CALL (pstorage_store, (&block_handle, ws16, 4, index) );
#ifdef PRINT_PENDING
	uint32_t count;
	BLE_CALL ( pstorage_access_status_get, (&count) );
	log(INFO, "Number of pending operations: %i", count);
#endif
}	

