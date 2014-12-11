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

//#include <BluetoothLE.h> // TODO: now for BLE_CALL, should be moved to other unit

extern "C" {
//#include <app_scheduler.h>

//#define PRINT_ITEMS
//#define PRINT_PENDING

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
		LOGi("Opcode %i executed (no error)", op_code);
	}
}

} // extern "C"

// NOTE: PSTORAGE_MAX_APPLICATIONS in pstorage_platform.h
//   has to be adjusted to the number of elements defined here

// NOTE: DO NOT CHANGE ORDER OF THE ELEMENTS OR THE FLASH
//   STORAGE WILL GET MESSED UP!! NEW ENTRIES ALWAYS AT THE END
static storage_config_t config[] {
	{PS_ID_POWER_SERVICE, {}, sizeof(ps_power_service_t)},
	{PS_ID_GENERAL_SERVICE, {}, sizeof(ps_general_service_t)}
};

#define NR_CONFIG_ELEMENTS SIZEOF_ARRAY(config)

Storage::Storage() {
	LOGi("Storage create");

	// call once before using any other API calls of the persistent storage module
	BLE_CALL(pstorage_init, ());

	for (int i = 0; i < NR_CONFIG_ELEMENTS; i++) {
		initBlocks(1, config[i].storage_size, config[i].handle);
	}
}

bool Storage::getHandle(ps_storage_id storageID, pstorage_handle_t& handle) {
	for (int i = 0; i < NR_CONFIG_ELEMENTS; i++) {
		if (config[i].id == storageID) {
			handle = config[i].handle;
			return true;
		}
	}
	return false;
}

/**
 * We allocate a single block of size "size". Biggest allocated size is 640 bytes.
 */
void Storage::initBlocks(int blocks, int size, pstorage_handle_t& handle) {
	LOGi("Create persistent storage (FLASH) of %i bytes", size);
	_size = size;

	// set parameter
	pstorage_module_param_t param;
	param.block_size = size;
	param.block_count = blocks;
	param.cb = pstorage_callback_handler;

	// register
	BLE_CALL ( pstorage_register, (&param, &handle) );
}

/**
 * We have to clear an entire block before we can write a value to one of the fields!
 */
void Storage::clearBlock(pstorage_handle_t handle, int blockID) {

	pstorage_handle_t block_handle;
	BLE_CALL ( pstorage_block_identifier_get,(&handle, blockID, &block_handle) );

	LOGw("Nordic bug: clear entire block before writing to it");
	BLE_CALL (pstorage_clear, (&block_handle, _size) );
}

// Get byte at location "index" 
void Storage::getUint8(pstorage_handle_t handle, int blockID, uint8_t *item) {
	static uint8_t wg8[4];
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, blockID, &block_handle) );
	BLE_CALL (pstorage_load, (wg8, &block_handle, 4, 0));
	memcpy(item, wg8, 1);
}

// Store byte
void Storage::setUint8(pstorage_handle_t handle, int blockID, const uint8_t item) {
	static uint8_t ws8[4];
	pstorage_handle_t block_handle;

//	clearBlock(handle, blockID);

	BLE_CALL ( pstorage_block_identifier_get, (&handle, blockID, &block_handle) );
	memcpy(ws8, &item, 1);
	BLE_CALL (pstorage_update, (&block_handle, ws8, 4, 0) );
}

// Get 16-bit integer
void Storage::getUint16(pstorage_handle_t handle, int blockID, uint16_t *item) {
	static uint8_t wg16[4];
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, blockID, &block_handle) );
	BLE_CALL (pstorage_load, (wg16, &block_handle, 4, 0) );
#ifdef PRINT_ITEMS
	LOGd("Load item[0]: %d", wg16[0]);
	LOGd("Load item[1]: %d", wg16[1]);
	LOGd("Load item[2]: %d", wg16[2]);
	LOGd("Load item[3]: %d", wg16[3]);
#endif
	memcpy(item, wg16, 2);
#ifdef PRINT_PENDINSG
	uint32_t count;
	BLE_CALL ( pstorage_access_status_get, (&count) );
	LOGd("Number of pending operations: %i", count);
#endif
}

// Set 16-bit integer
void Storage::setUint16(pstorage_handle_t handle, int blockID, const uint16_t item) {
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, blockID, &block_handle) );

//	clearBlock(handle, blockID);

	static uint8_t ws16[4];
	memcpy(ws16, &item, 2);
#ifdef PRINT_ITEMS
	LOGd("Store item[0]: %d", ws16[0]);
	LOGd("Store item[1]: %d", ws16[1]);
	LOGd("Store item[2]: %d", ws16[2]);
	LOGd("Store item[3]: %d", ws16[3]);
#endif
	BLE_CALL (pstorage_update, (&block_handle, ws16, 4, 0) );
#ifdef PRINT_PENDING
	uint32_t count;
	BLE_CALL ( pstorage_access_status_get, (&count) );
	LOGd("Number of pending operations: %i", count);
#endif
}	

void Storage::getString(pstorage_handle_t handle, int blockID, char* item, int16_t length) {
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, blockID, &block_handle) );

	BLE_CALL (pstorage_load, ((uint8_t*)item, &block_handle, length, 0) );
}

void Storage::setString(pstorage_handle_t handle, int blockID, const char* item, int16_t length) {
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, blockID, &block_handle) );

//	clearBlock(handle, blockID);

	BLE_CALL (pstorage_update, (&block_handle, (uint8_t*)item, length, 0) );
}

void Storage::getStruct(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size) {
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, 0, &block_handle) );

	BLE_CALL (pstorage_load, ((uint8_t*)item, &block_handle, size, 0) );
}

void Storage::setStruct(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size) {
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, 0, &block_handle) );

	//	clearBlock(handle, blockID);

	BLE_CALL (pstorage_update, (&block_handle, (uint8_t*)item, size, 0) );

}
