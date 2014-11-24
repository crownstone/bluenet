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

static void pstorage_callback_handler(pstorage_handle_t * handle, uint8_t op_code, uint32_t result, uint8_t * p_data, 
		uint32_t data_len) {

	// we might want to check if things are actually stored, by using this callback	
	//if (op_code == PSTORAGE_STORE_OP_CODE) {
	//}
	// do nothing
}

Storage::Storage() {
}

Storage::~Storage() {
}

/**
 * We allocate a single block of size "size". Biggest allocated size is 640 bytes.
 */
bool Storage::init(int size) {
	uint32_t err_code;

	// call once before using any other API calls of the persistent storage module
	pstorage_init();

	// set parameter
	pstorage_module_param_t param;
	param.block_size = size;
	param.block_count = 1;
	param.cb = pstorage_callback_handler;

	// register
	err_code = pstorage_register(&param, handle);

	return (err_code == NRF_SUCCESS);
}

