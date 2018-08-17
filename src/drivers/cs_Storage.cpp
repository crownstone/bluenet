/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/*
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/
 *       Writing to persistent memory should be done between connection/advertisement events...
 */

/*
 * For more information see:
 * http://developer.nordicsemi.com/nRF51_SDK/doc/7.0.1/s110/html/a00763.html#ga0a57b964c8945eaca2d267835ef6688c
 */

#include <climits>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <float.h>
#include <mesh/cs_Mesh.h>
#include <protocol/cs_ConfigTypes.h>
#include <protocol/cs_StateTypes.h>
#include <storage/cs_State.h>

extern "C" {
	// Define event handler for errors during initialization
	static void fds_evt_handler(fds_evt_t const * p_fds_evt) {
		Storage::getInstance().handleFileStorageEvent(p_fds_evt);
	}
}

#define NR_CONFIG_ELEMENTS SIZEOF_ARRAY(config)

Storage::Storage() : EventListener() {
	LOGd(FMT_CREATE, "Storage");

	EventDispatcher::getInstance().addListener(this);
}

ret_code_t Storage::remove(st_file_id_t file_id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	ret_code_t ret_code;
	ret_code = fds_file_delete(file_id);
	return ret_code;
}


ret_code_t Storage::init() {
	LOGd(FMT_INIT, "storage");

	// clear fds token before first use 
	memset(&_ftok, 0x00, sizeof(fds_find_token_t));

	// register and initialize module
	ret_code_t ret_code;
	ret_code = fds_register(fds_evt_handler);
	if (ret_code != FDS_SUCCESS) {
		LOGe("Registering FDS event handler failed (err=%i)", ret_code);
		return ret_code;
	}
	ret_code = fds_init();
	if (ret_code != FDS_SUCCESS) {
		LOGe("Init FDS failed (err=%i)", ret_code);
		return ret_code;
	}
	return ret_code;
}

ret_code_t Storage::garbageCollect() {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	ret_code_t ret_code;
	LOGd("Perform garbage collection");
	ret_code = fds_gc();
	if (ret_code != FDS_SUCCESS) {
		LOGw("No success (err=%i)", ret_code);
	}
	return ret_code;
}

ret_code_t Storage::write(st_file_id_t file_id, st_file_data_t file_data) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	fds_record_t        record;
	fds_record_desc_t   record_desc;
	ret_code_t ret_code;

	record.file_id           = file_id;
	record.key               = +file_data.type;
	record.data.p_data       = file_data.value;
	record.data.length_words = file_data.size; 

	if (exists(file_id, file_data.type, record_desc)) {
		LOGd("Update file %i record %i", file_id, record.key);
		ret_code = fds_record_update(&record_desc, &record);
		FDS_ERROR_CHECK(ret_code);
	}
	else {
		LOGd("Write file %i, record %i, ptr %p", file_id, record.key, record.data.p_data);
		ret_code = fds_record_write(&record_desc, &record);
		FDS_ERROR_CHECK(ret_code);
		static bool garbage_collection = false;
		switch(ret_code) {
			case FDS_ERR_NO_SPACE_IN_FLASH:
				if (!garbage_collection) {
					garbage_collection = true;
					garbageCollect();
					// try again, just once
					ret_code = write(file_id, file_data);
					garbage_collection = false;
				}
				break;
			case FDS_SUCCESS:
				LOGd("Write successful");
				break;
		}
	}
	return ret_code;
}

void Storage::print(const std::string & prefix, CS_TYPE type) {
	LOGd("%s %s (%i)", prefix.c_str(), TypeName(type), type);
}

ret_code_t Storage::read(st_file_id_t file_id, st_file_data_t file_data) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	fds_flash_record_t flash_record;
	fds_record_desc_t  record_desc;
	ret_code_t         ret_code;

	ret_code = FDS_ERR_NOT_FOUND;

	//LOGd("Read from file %i record %i", file_id, file_data.key);
	// go through all records with given file_id and key (can be multiple)
	while (fds_record_find(file_id, +file_data.type, &record_desc, &_ftok) == FDS_SUCCESS) {
		ret_code = fds_record_open(&record_desc, &flash_record);
		if (ret_code != FDS_SUCCESS) break;
		print("Found: ", file_data.type);

		// map flash_record.p_data to value
		file_data.size = flash_record.p_header->length_words;
		LOGd("Record has size %i", file_data.size);
		memcpy(file_data.value, flash_record.p_data, file_data.size);

		// invalidates the record	
		ret_code = fds_record_close(&record_desc);
		if (ret_code != FDS_SUCCESS) break;
	}
	return ret_code;
}

ret_code_t Storage::remove(st_file_id_t file_id, CS_TYPE type) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	fds_record_desc_t record_desc;
	ret_code_t        ret_code;

	ret_code = FDS_ERR_NOT_FOUND;

	// go through all records with given file_id and key (can be multiple)
	while (fds_record_find(file_id, +type, &record_desc, &_ftok) == FDS_SUCCESS) {
		ret_code = fds_record_delete(&record_desc);
		if (ret_code != FDS_SUCCESS) break;
	}
	return ret_code;
}

bool Storage::exists(st_file_id_t file_id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	// not yet implemented
	return false;
}

bool Storage::exists(st_file_id_t file_id, CS_TYPE type) {
	fds_record_desc_t record_desc;
	return exists(file_id, type, record_desc);
}

bool Storage::exists(st_file_id_t file_id, CS_TYPE type, fds_record_desc_t record_fd) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	while (fds_record_find(file_id, +type, &record_fd, &_ftok) == FDS_SUCCESS) {
		print("Exists:", type);
		return true;
	}
	print("Does not exist:", type);
	return false;
}

void Storage::handleFileStorageEvent(fds_evt_t const * p_fds_evt) {
	switch (p_fds_evt->id) {
		case FDS_EVT_INIT:
			if (p_fds_evt->result != FDS_SUCCESS) {
				LOGe("Storage: Initialization failed");
			} else {
				LOGd("Storage: Initialization successful");
				_initialized = true;
			}
			break;
		case FDS_EVT_WRITE:
			if (p_fds_evt->result != FDS_SUCCESS) {
				LOGe("Storage: write failed");
			}
			break;
		case FDS_EVT_UPDATE:
			if (p_fds_evt->result != FDS_SUCCESS) {
				LOGe("Storage: update failed");
			}
			break;
		case FDS_EVT_DEL_RECORD:
			if (p_fds_evt->result != FDS_SUCCESS) {
				LOGe("Storage: delete record failed");
			}
			break;
		case FDS_EVT_DEL_FILE:
			if (p_fds_evt->result != FDS_SUCCESS) {
				LOGe("Storage: delete file failed");
			}
			break;
		case FDS_EVT_GC:
			if (p_fds_evt->result != FDS_SUCCESS) {
				LOGe("Storage: garbage collection failed");
			}
			break;
		default:
			LOGd("Storage evt %i with value %i", p_fds_evt->id, p_fds_evt->result);
			break;
	}

	switch(p_fds_evt->result) {
		case FDS_ERR_NOT_FOUND:
			LOGe("Not found");
			break;
		case FDS_SUCCESS:
			break;
		default:
			LOGe("Error value is: %i", p_fds_evt->result);
			break;
	}
}

