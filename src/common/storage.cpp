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
#include <limits.h>

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
		LOGi("Init %i bytes persistent storage (FLASH) for id %d", config[i].storage_size, config[i].id);
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

void Storage::setString(std::string value, char* target) {
	if (value.length() < MAX_STRING_SIZE) {
		memset(target, 0, MAX_STRING_SIZE);
		memcpy(target, value.c_str(), value.length());
	} else {
		LOGe("string too long!!");
	}
}

// helper function to get std::string from char array, or default value
// if the value read is empty, unassigned (filled with FF) or too long
void Storage::getString(char* value, std::string& target, std::string default_value) {

#ifdef PRINT_ITEMS
	LOGi("raw value: %s", value);
	_log(INFO, "stored value: ");
	for (int i = 0; i < MAX_STRING_SIZE; i++) {
		_log(INFO, "%X ", value[i]);
	}
	_log(INFO, "\r\n");
#endif

	target = std::string(value);
	// if the last char is equal to FF that means the memory
	// is new and has not yet been written to, so we use the
	// default value. same if the stored value is an empty string
	if (target == "" || value[MAX_STRING_SIZE-1] == 0xFF) {
		LOGd("use default value");
		target = default_value;
	} else {
		LOGd("found stored value: %s", target.c_str());
	}
}

void Storage::setUint8(uint8_t value, uint32_t& target) {
	target = value;
}

void Storage::getUint8(uint32_t value, uint8_t& target, uint8_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGi("raw value: %02X %02X %02X %02X", tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last byte is FF which means that memory is unnassigned
	// and value has to be ignored
	if (value & (0xFF << 3)) {
		LOGd("use default value");
		target = default_value;
	} else {
		target = value;
		LOGd("found stored value: %d", target);
	}
}

void Storage::setUint16(uint16_t value, uint32_t& target) {
	target = value;
}

void Storage::getUint16(uint32_t value, uint16_t& target, uint16_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGi("raw value: %02X %02X %02X %02X", tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last byte is FF which means that memory is unnassigned
	// and value has to be ignored
	if (value & (0xFF << 3)) {
		LOGd("use default value");
		target = default_value;
	} else {
		target = value;
		LOGd("found stored value: %d", target);
	}
}

void Storage::setUint32(uint32_t value, uint32_t& target) {
	if (value & 1 << 31) {
		LOGe("value %d too big, can only write max %d", value, INT_MAX & ~(1 << 31));
	} else {
		target = value;
	}
}

void Storage::getUint32(uint32_t value, uint32_t& target, uint32_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGi("raw value: %02X %02X %02X %02X", tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last bit is 1 which means that memory is unnassigned
	// and value has to be ignored
	if (value & (1 << 31)) {
		LOGd("use default value");
		target = default_value;
	} else {
		target = value;
		LOGd("found stored value: %d", target);
	}
}
