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
#include <common/cs_Handlers.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <float.h>
#include <mesh/cs_Mesh.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>

#define NR_CONFIG_ELEMENTS SIZEOF_ARRAY(config)

Storage::Storage() : EventListener() {
	LOGd(FMT_CREATE, "storage");

	EventDispatcher::getInstance().addListener(this);
}

ret_code_t Storage::remove(st_file_id_t file_id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	LOGi("Delete file, removes all configuration values!");
	ret_code_t ret_code;
	ret_code = fds_file_delete(file_id);
	return ret_code;
}

void Storage::initSearch() {
	// clear fds token before every use
	memset(&_ftok, 0x00, sizeof(fds_find_token_t));
}

ret_code_t Storage::init() {
	LOGi(FMT_INIT, "storage");
	ret_code_t ret_code;

	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}

	// register and initialize module
	LOGd("fds_register");
	ret_code = fds_register(fds_evt_handler);
	if (ret_code != FDS_SUCCESS) {
		LOGe("Registering FDS event handler failed (err=%i)", ret_code);
		return ret_code;
	}
		

	LOGd("fds_init");
	ret_code = fds_init();
	if (ret_code != FDS_SUCCESS) {
		LOGe("Init FDS failed (err=%i)", ret_code);
		return ret_code;
	}
//	LOGd("Storage init done (wait for FDS_EVT_INIT)");

	/*
	while (!_initialized)
	{
		(void) sd_app_evt_wait();
	}*/

	return ret_code;
}

ret_code_t Storage::garbageCollect() {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	ret_code_t ret_code;
	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}
	LOGnone("fds_gc");
	ret_code = fds_gc();
	if (ret_code != FDS_SUCCESS) {
		LOGw("No success garbage collection (err=%i)", ret_code);
	}
	LOGnone("Garbage collected");
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
	record.key               = to_underlying_type(file_data.type);
	record.data.p_data       = file_data.value;
	record.data.length_words = file_data.size >> 2; 
	LOGnone("Store 0x%x of size %i", record.data.p_data, record.data.length_words);

	bool f_exists = false;
	ret_code = exists(file_id, file_data.type, record_desc, f_exists);
	if (f_exists) {
		LOGd("Update file %i record %i", file_id, record.key);
		ret_code = fds_record_update(&record_desc, &record);
		FDS_ERROR_CHECK(ret_code);
	}
	else {
		LOGnone("Write file %i, record %i, ptr %p", file_id, record.key, record.data.p_data);
		ret_code = fds_record_write(&record_desc, &record);
		FDS_ERROR_CHECK(ret_code);
		static bool garbage_collection = false;
		f_exists = false;
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
				LOGnone("Write successful");
				ret_code = exists(file_id, file_data.type, record_desc, f_exists);
				if (!f_exists) {
					LOGw("Warning: written with delay");
				}
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
	initSearch();
	bool break_on_first = false; 
	bool found = false;
	while (fds_record_find(file_id, to_underlying_type(file_data.type), &record_desc, &_ftok) == FDS_SUCCESS) {

		LOGnone("Read record %i", +file_data.type);
		if (!found) {
			ret_code = fds_record_open(&record_desc, &flash_record);
			if (ret_code != FDS_SUCCESS) {
				LOGw("Error on opening record");
				break;
			}
			found = true;

			// map flash_record.p_data to value
			file_data.size = flash_record.p_header->length_words;
			memcpy(file_data.value, flash_record.p_data, file_data.size);

			// invalidates the record	
			ret_code = fds_record_close(&record_desc);
			if (ret_code != FDS_SUCCESS) {
				LOGw("Error on closing record");
			}

			if (break_on_first) {
				break;
			}
		}
	} 

	if (!found) {
		LOGnone("Record not found");
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
	initSearch();
	while (fds_record_find(file_id, to_underlying_type(type), &record_desc, &_ftok) == FDS_SUCCESS) {
		ret_code = fds_record_delete(&record_desc);
		if (ret_code != FDS_SUCCESS) break;
	}
	return ret_code;
}

ret_code_t Storage::exists(st_file_id_t file_id, bool & result) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	// not yet implemented
	return false;
}

ret_code_t Storage::exists(st_file_id_t file_id, CS_TYPE type, bool & result) {
	fds_record_desc_t record_desc;
	return exists(file_id, type, record_desc, result);
}

/**
 * Check if a record exists and removes duplicate record items. We only support one record field.
 */
ret_code_t Storage::exists(st_file_id_t file_id, CS_TYPE type, fds_record_desc_t & record_desc, bool & result) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	initSearch();
	result = false;
	while (fds_record_find(file_id, to_underlying_type(type), &record_desc, &_ftok) == FDS_SUCCESS) {
		if (result) {
			// remove all subsequent records...
			LOGd("Delete record %i desc %i", to_underlying_type(type), &record_desc);
			fds_record_delete(&record_desc);
		}
		if (!result) {
			//print("Exists:", type);
			result = true;
		}
	}
	if (!result) {
		//print("Does not exist:", type);
	}
	return ERR_SUCCESS;
}

/**
 * Receives only successful events via the callback handler registered in the SoftDevice. On the moment we propagate
 * successful write events so it can be used by other modules to respond to.
 */
void Storage::handleSuccessfulEvent(fds_evt_t const * p_fds_evt) {
	switch (p_fds_evt->id) {
	case FDS_EVT_INIT:
		LOGd("Successfully initialized");
		_initialized = true;
		break;
	case FDS_EVT_WRITE:
	case FDS_EVT_UPDATE: {
		uint8_t record_key = p_fds_evt->write.record_key;
		LOGd("Dispatch write/update event, record %i", record_key);
		event_t event1(CS_TYPE::EVT_STORAGE_WRITE_DONE, (void*)&record_key, sizeof(record_key));
		EventDispatcher::getInstance().dispatch(event1);
		break;
	}
	case FDS_EVT_DEL_RECORD:
		LOGi("Record successfully deleted");
		break;
	case FDS_EVT_DEL_FILE:
		LOGi("File successfully deleted");
		fds_gc();
		break;
	default:
		LOGd("Storage evt %i with value %i", p_fds_evt->id, p_fds_evt->result);
		break;
	}
}

/**
 * The p_fds_evt struct has the following structure:
 *   id,
 *   result,
 *   write { record_id, file_id, record_key, is_record_updated },
 *   del { record_id, file_id, record_key }
 */
void Storage::handleFileStorageEvent(fds_evt_t const * p_fds_evt) {

	LOGnone("FS: %i: %i", p_fds_evt->result, p_fds_evt->id);
	switch(p_fds_evt->result) {
	case FDS_ERR_NOT_FOUND:
		LOGe("Not found on %s (%i)", NordicFDSEventTypeName(p_fds_evt->id), p_fds_evt->id);
		LOGe("Record %s (%i)", TypeName(static_cast<CS_TYPE>(p_fds_evt->write.record_id)), p_fds_evt->write.record_id);
		break;
	case FDS_SUCCESS: 
		handleSuccessfulEvent(p_fds_evt);
		break;
	default:
		LOGe("Storage error: %i", p_fds_evt->result);
		break;
	}
}

