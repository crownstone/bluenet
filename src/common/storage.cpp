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
#include <app_scheduler.h>

static pstorage_handle_t handle;
static pstorage_handle_t block_handle;

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

static uint8_t test[2];

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

	log(INFO, "Get block identifier");
	err_code = pstorage_block_identifier_get(&handle, 0, &block_handle);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}

#define TEST_CLEAR
#define TEST_WRITE

#ifdef TEST_CLEAR
	log(INFO, "Clear single block");
	err_code = pstorage_clear(&handle, size);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}
#endif

#ifdef TEST_WRITE	
	test[0] = 12;
	test[1] = 13;
	log(INFO, "Write something");
	//uint16_t test = 12;
	pstorage_store(&handle, test, 2, 0);
	//setUint16(0, &test);
	app_sched_execute();
#endif
	uint32_t count;
	err_code = pstorage_access_status_get(&count);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}
	log(INFO, "Number of pending operations: %i", count);

	return (err_code == NRF_SUCCESS);
}

// Get byte at location "index" 
void Storage::getUint8(int index, uint8_t *item) {
	pstorage_load(item, &block_handle, 1, index); 
}

// Store byte
void Storage::setUint8(int index, uint8_t *item) {
	pstorage_store(&block_handle, item, 1, index);
}

// Get 16-bit integer
void Storage::getUint16(int index, uint16_t *item) {
	uint32_t err_code;
	uint32_t count;
	err_code = pstorage_access_status_get(&count);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}
	log(INFO, "Number of pending operations: %i", count);
	pstorage_load((uint8_t*)item, &block_handle, 2, index); //todo: check endianness
	app_sched_execute();
}

// Set 16-bit integer
void Storage::setUint16(int index, const uint16_t *item) {
	pstorage_store(&block_handle, (uint8_t*)item, 2, index);
	uint32_t err_code;
	uint32_t count;
	/*
	uint32_t size = 32;
	log(INFO, "Clear single block");
	err_code = pstorage_clear(&block_handle, size);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	}*/
	err_code = pstorage_access_status_get(&count);
	if (err_code != NRF_SUCCESS) {
		LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		APP_ERROR_CHECK(err_code);
	} 
	log(INFO, "Number of pending operations: %i", count);
}	

