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
#include "drivers/cs_Storage.h"

#include <climits>
#include <string>

#include "services/cs_IndoorLocalisationService.h"
#include "services/cs_GeneralService.h"
#include "services/cs_PowerService.h"

#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"
#include "util/cs_BleError.h"

extern "C" {

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

// NOTE: DO NOT CHANGE ORDER OF THE ELEMENTS OR THE FLASH
//   STORAGE WILL GET MESSED UP!! NEW ENTRIES ALWAYS AT THE END
static storage_config_t config[] {
	{PS_ID_CONFIGURATION, {}, sizeof(ps_configuration_t)},
	{PS_ID_INDOORLOCALISATION_SERVICE, {}, sizeof(ps_indoorlocalisation_service_t)}
};

#define NR_CONFIG_ELEMENTS SIZEOF_ARRAY(config)

Storage::Storage() {
	LOGi("Storage create");

	// call once before using any other API calls of the persistent storage module
	BLE_CALL(pstorage_init, ());

	for (int i = 0; i < NR_CONFIG_ELEMENTS; i++) {
		LOGi("Init %i bytes persistent storage (FLASH) for id %d", config[i].storage_size, config[i].id);
		initBlocks(config[i].storage_size, config[i].handle);
	}
}

storage_config_t* Storage::getStorageConfig(ps_storage_id storageID) {

	for (int i = 0; i < NR_CONFIG_ELEMENTS; i++) {
		if (config[i].id == storageID) {
			return &config[i];
		}
	}

	return NULL;
}

bool Storage::getHandle(ps_storage_id storageID, pstorage_handle_t& handle) {
	storage_config_t* config;

	if ((config = getStorageConfig(storageID))) {
		handle = config->handle;
		return true;
	} else {
		return false;
	}
}

/**
 * We allocate a single block of size "size". Biggest allocated size is 640 bytes.
 */
void Storage::initBlocks(pstorage_size_t size, pstorage_handle_t& handle) {
	// set parameter
	pstorage_module_param_t param;
	param.block_size = size;
	param.block_count = 1;
	param.cb = pstorage_callback_handler;

	// register
	BLE_CALL ( pstorage_register, (&param, &handle) );
}

/**
 * Nordic bug: We have to clear the entire block!
 */
void Storage::clearBlock(pstorage_handle_t handle, ps_storage_id storageID) {

	storage_config_t* config;
	if (!(config = getStorageConfig(storageID))) {
		// if no config was found for the given storage ID return
		return;
	}

	pstorage_handle_t block_handle;
	BLE_CALL ( pstorage_block_identifier_get,(&handle, 0, &block_handle) );

	LOGw("Nordic bug: clear entire block before writing to it");
	BLE_CALL (pstorage_clear, (&block_handle, config->storage_size) );
}

void Storage::readStorage(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size) {
	pstorage_handle_t block_handle;

	BLE_CALL ( pstorage_block_identifier_get, (&handle, 0, &block_handle) );

	BLE_CALL (pstorage_load, ((uint8_t*)item, &block_handle, size, 0) );

#ifdef PRINT_ITEMS
	uint8_t* ptr = (uint8_t*)item;
	_log(INFO, "getStruct: ");
	for (int i = 0; i < size; i++) {
		_log(INFO, "%X ", ptr[i]);
	}
	_log(INFO, "\r\n");
#endif

}

void Storage::writeStorage(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size) {
	pstorage_handle_t block_handle;

#ifdef PRINT_ITEMS
	uint8_t* ptr = (uint8_t*)item;
	_log(INFO, "set struct: ");
	for (int i = 0; i < size; i++) {
		_log(INFO, "%X ", ptr[i]);
	}
	_log(INFO, "\r\n");
#endif

	BLE_CALL ( pstorage_block_identifier_get, (&handle, 0, &block_handle) );

	//	clearBlock(handle);

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
	_log(INFO, "get string (raw) : ");
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
#ifdef PRINT_ITEMS
		LOGd("use default value");
#endif
		target = default_value;
	} else {
#ifdef PRINT_ITEMS
		LOGd("found stored value: %s", target.c_str());
#endif
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
#ifdef PRINT_ITEMS
		LOGd("use default value");
#endif
		target = default_value;
	} else {
		target = value;
#ifdef PRINT_ITEMS
		LOGd("found stored value: %d", target);
#endif
	}
}

void Storage::setInt8(int8_t value, int32_t& target) {
	target = value;
}

void Storage::getInt8(int32_t value, int8_t& target, int8_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGi("raw value: %02X %02X %02X %02X", tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last byte is FF which means that memory is unnassigned
	// and value has to be ignored
	if (value & (0xFF << 3)) {
#ifdef PRINT_ITEMS
		LOGd("use default value");
#endif
		target = default_value;
	} else {
		target = value;
#ifdef PRINT_ITEMS
		LOGd("found stored value: %d", target);
#endif
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
#ifdef PRINT_ITEMS
		LOGd("use default value");
#endif
		target = default_value;
	} else {
		target = value;

#ifdef PRINT_ITEMS
		LOGd("found stored value: %d", target);
#endif
	}
}

void Storage::setUint32(uint32_t value, uint32_t& target) {
	if (value == INT_MAX) {
		LOGe("value %d too big, can only write max %d", value, INT_MAX-1);
	} else {
		target = value;
	}
}

void Storage::getUint32(uint32_t value, uint32_t& target, uint32_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGi("raw value: %02X %02X %02X %02X", tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if value is equal to INT_MAX (FFFFFFFF) which means that memory is
	// unnassigned and value has to be ignored
	if (value == INT_MAX) {
#ifdef PRINT_ITEMS
		LOGd("use default value");
		target = default_value;
#endif
	} else {
		target = value;
#ifdef PRINT_ITEMS
		LOGd("found stored value: %d", target);
#endif
	}
}

//template<typename T>
//void Storage::setArray(T* src, T* dest, uint16_t length) {
//	bool isUnassigned = true;
//	for (int i = 0; i < length; ++i) {
//		if (src[i] != 0xFF) {
//			isUnassigned = false;
//			break;
//		}
//	}
//
//	if (isUnassigned) {
//		LOGe("cannot write all FF");
//	} else {
//		memccpy(dest, src, 0, length * sizeof(T));
//	}
//}
//
//template<typename T>
//void Storage::getArray(T* src, T* dest, T* default_value, uint16_t length) {
//
//#ifdef PRINT_ITEMS
//	_log(INFO, "raw value: ");
//	for (int i = 0; i < length; ++i) {
//		_log(INFO, "%X", src[i]);
//	}
//	_log(INFO, "\r\n");
//#endif
//
//	bool isUnassigned = true;
//	for (int i = 0; i < length; ++i) {
//		if (src[i] != 0xFF) {
//			isUnassigned = false;
//			break;
//		}
//	}
//
//	if (isUnassigned) {
//		LOGd("use default value");
//		memccpy(dest, default_value, 0, length * sizeof(T));
//	} else {
//		memccpy(dest, src, 0, length * sizeof(T));
//	}
//
//}
