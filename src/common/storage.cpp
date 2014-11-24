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

extern "C" {
static pstorage_handle_t handle;

static void pstorage_callback_handler(pstorage_handle_t * handle, uint8_t op_code, uint32_t result, uint8_t * p_data, 
		uint32_t data_len) {
	log(INFO, "Opcode %i", op_code);
	// we might want to check if things are actually stored, by using this callback	
	if (op_code == PSTORAGE_LOAD_OP_CODE) {
		LOGd("Error with loading data");
	}
	// do nothing
	if (result != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", result, result);
		APP_ERROR_CHECK(result);
	}
}
}

Storage::Storage() {
}

Storage::~Storage() {
}

/**
 * We allocate a single block of size "size". Biggest allocated size is 640 bytes.
 */
bool Storage::init(int size) {
	log(INFO, "Create persistent storage of size %i", size);
	uint32_t err_code;

	// call once before using any other API calls of the persistent storage module
	err_code = pstorage_init();
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}

	// set parameter
	pstorage_module_param_t param;
	param.block_size = size;
	param.block_count = 1;
	param.cb = pstorage_callback_handler;

	// register
	log(INFO, "Register");
	err_code = pstorage_register(&param, &handle);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}

	log(INFO, "Write something");
	uint16_t test = 12;
	setUint16(0, &test);
	
	return (err_code == NRF_SUCCESS);
}

// Get byte at location "index" 
void Storage::getUint8(int index, uint8_t *item) {
	pstorage_load(item, &handle, 1, index); 
}

// Store byte
void Storage::setUint8(int index, uint8_t *item) {
	pstorage_store(&handle, item, 1, index);
}

// Get 16-bit integer
void Storage::getUint16(int index, uint16_t *item) {
	pstorage_load((uint8_t*)item, &handle, 2, index); //todo: check endianness
}

// Set 16-bit integer
void Storage::setUint16(int index, const uint16_t *item) {
	pstorage_store(&handle, (uint8_t*)item, 2, index);
}	

