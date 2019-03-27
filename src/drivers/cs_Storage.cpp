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

cs_ret_code_t Storage::init() {
	LOGi(FMT_INIT, "storage");
	ret_code_t fds_ret_code;

	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}

	// register and initialize module
	LOGd("fds_register");
	fds_ret_code = fds_register(fds_evt_handler);
	if (fds_ret_code != FDS_SUCCESS) {
		LOGe("Registering FDS event handler failed (err=%i)", fds_ret_code);
		return getErrorCode(fds_ret_code);
	}


	LOGd("fds_init");
	fds_ret_code = fds_init();
	if (fds_ret_code != FDS_SUCCESS) {
		LOGe("Init FDS failed (err=%i)", fds_ret_code);
		return getErrorCode(fds_ret_code);
	}
//	LOGd("Storage init done (wait for FDS_EVT_INIT)");

	/*
	while (!_initialized)
	{
		(void) sd_app_evt_wait();
	}*/

	return getErrorCode(fds_ret_code);
}

cs_ret_code_t Storage::garbageCollect() {
	return getErrorCode(garbageCollectInternal());
}

ret_code_t Storage::garbageCollectInternal() {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	ret_code_t fds_ret_code;
	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}
	LOGnone("fds_gc");
	fds_ret_code = fds_gc();
	if (fds_ret_code != FDS_SUCCESS) {
		LOGw("No success garbage collection (err=%i)", fds_ret_code);
	}
	LOGd("Garbage collected");
	return fds_ret_code;
}

cs_ret_code_t Storage::write(cs_file_id_t file_id, const cs_state_data_t & file_data) {
	return getErrorCode(writeInternal(file_id, file_data));
}

ret_code_t Storage::writeInternal(cs_file_id_t file_id, const cs_state_data_t & file_data) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	fds_record_t record;
	fds_record_desc_t record_desc;
	ret_code_t fds_ret_code;

	record.file_id           = file_id;
	record.key               = to_underlying_type(file_data.type);
	record.data.p_data       = file_data.value;
	// Assume the allocation was done by storage.
	// Size is in bytes, each word is 4B.
	record.data.length_words = getPaddedSize(file_data.size) >> 2;
	LOGd("Store %p of word size %u", record.data.p_data, record.data.length_words);

	bool f_exists = false;
	fds_ret_code = exists(file_id, file_data.type, record_desc, f_exists);
	if (f_exists) {
		LOGd("Update file %u record %u", file_id, record.key);
		fds_ret_code = fds_record_update(&record_desc, &record);
		FDS_ERROR_CHECK(fds_ret_code);
	}
	else {
		LOGd("Write file %u, record %u, ptr %p", file_id, record.key, record.data.p_data);
		fds_ret_code = fds_record_write(&record_desc, &record);
		FDS_ERROR_CHECK(fds_ret_code);
		static bool garbage_collection = false; // TODO: why not a member variable?
		f_exists = false;
		switch(fds_ret_code) {
			case FDS_ERR_NO_SPACE_IN_FLASH:
				if (!garbage_collection) {
					// TODO: is this going to help? Garbage collection is asynch, so we retry to write before GC is done.
					garbage_collection = true;
					garbageCollect();
					// try again, just once
					fds_ret_code = write(file_id, file_data);
					garbage_collection = false;
				}
				break;
			case FDS_SUCCESS:
				LOGnone("Write successful");
				fds_ret_code = exists(file_id, file_data.type, record_desc, f_exists); // TODO: why do we check if file exists again? Doesn't this always returns false?
				if (!f_exists) {
					LOGw("Warning: written with delay");
				}
				break;
		}
	}
	return fds_ret_code;
}

/**
 * Maybe we should check if data is stored at right boundary.
 *
 * if ((uint32_t)data.value % 4u != 0) {
 *		LOGe("Unaligned type: %s: %p", TypeName(type), data.value);
 *	}
 */
uint8_t* Storage::allocate(size16_t& size) {
	size16_t flashSize = getPaddedSize(size);
	size16_t paddingSize = flashSize - size;
	uint8_t* ptr = (uint8_t*) malloc(sizeof(uint8_t) * flashSize);
	memset(ptr + size, 0xFF, paddingSize);
	size = flashSize;
	return ptr;
}

size16_t Storage::getPaddedSize(size16_t size) {
	size16_t flashSize = CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(size, 4);
	return flashSize;
}

void Storage::print(const std::string & prefix, CS_TYPE type) {
	LOGd("%s %s (%i)", prefix.c_str(), TypeName(type), type);
}

cs_ret_code_t Storage::read(cs_file_id_t file_id, cs_state_data_t & file_data) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	fds_flash_record_t flash_record;
	fds_record_desc_t record_desc;
	ret_code_t fds_ret_code;

	fds_ret_code = FDS_ERR_NOT_FOUND;
	cs_ret_code_t cs_ret_code = ERR_SUCCESS;

	//LOGd("Read from file %i record %i", file_id, file_data.key);
	// go through all records with given file_id and key (can be multiple)
	initSearch();
	bool break_on_first = true;
	bool found = false;
	while (fds_record_find(file_id, to_underlying_type(file_data.type), &record_desc, &_ftok) == FDS_SUCCESS) {
		LOGd("Read record %u", to_underlying_type(file_data.type));
		if (!found) {
			fds_ret_code = fds_record_open(&record_desc, &flash_record);
			if (fds_ret_code != FDS_SUCCESS) {
				LOGw("Error on opening record");
				break;
			}
			found = true;

			// map flash_record.p_data to value
			size_t flashSize = flash_record.p_header->length_words << 2; // Size is in bytes, each word is 4B.
			if (flashSize != getPaddedSize(file_data.size)) {
				LOGe("stored size = %u ram size = %u", flashSize, file_data.size);
				// TODO: remove this record?
				cs_ret_code = ERR_WRONG_PAYLOAD_LENGTH;
			}
			else {
				memcpy(file_data.value, flash_record.p_data, file_data.size);
			}

			// invalidates the record
			fds_ret_code = fds_record_close(&record_desc);
			if (fds_ret_code != FDS_SUCCESS) {
				// TODO: maybe still return success? But how to handle the close error?
				LOGe("Error on closing record");
			}

			if (break_on_first) {
				break;
			}
		}
	}
	if (!found) {
		LOGd("Record not found");
	}
	if (cs_ret_code != ERR_SUCCESS) {
		return cs_ret_code;
	}
	return getErrorCode(fds_ret_code);
}

cs_ret_code_t Storage::remove(cs_file_id_t file_id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	LOGi("Delete file, removes all configuration values!");
	ret_code_t fds_ret_code;
	fds_ret_code = fds_file_delete(file_id);
	return getErrorCode(fds_ret_code);
}

cs_ret_code_t Storage::remove(cs_file_id_t file_id, CS_TYPE type) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	fds_record_desc_t record_desc;
	ret_code_t fds_ret_code;

	fds_ret_code = FDS_ERR_NOT_FOUND;

	// go through all records with given file_id and key (can be multiple)
	initSearch();
	while (fds_record_find(file_id, to_underlying_type(type), &record_desc, &_ftok) == FDS_SUCCESS) {
		fds_ret_code = fds_record_delete(&record_desc);
		if (fds_ret_code != FDS_SUCCESS) break;
	}
	return getErrorCode(fds_ret_code);
}

//cs_ret_code_t Storage::exists(cs_file_id_t file_id, bool & result) {
//	if (!_initialized) {
//		LOGe("Storage not initialized");
//		return ERR_NOT_INITIALIZED;
//	}
//	// TODO: not yet implemented
//	return false;
//}

ret_code_t Storage::exists(cs_file_id_t file_id, CS_TYPE type, bool & result) {
	fds_record_desc_t record_desc;
	return exists(file_id, type, record_desc, result);
}

/**
 * Check if a record exists and removes duplicate record items. We only support one record field.
 * TODO: why is duplicate removal part of the function "exists" ?
 */
ret_code_t Storage::exists(cs_file_id_t file_id, CS_TYPE type, fds_record_desc_t & record_desc, bool & result) {
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

void Storage::initSearch() {
	// clear fds token before every use
	memset(&_ftok, 0x00, sizeof(fds_find_token_t));
}

/**
 * Returns cs_ret_code_t given a ret_code_t from FDS.
 */
cs_ret_code_t Storage::getErrorCode(ret_code_t code) {
	switch (code) {
	case FDS_SUCCESS:
		return ERR_SUCCESS;
	case FDS_ERR_NOT_FOUND:
		return ERR_NOT_FOUND;
	default:
		return ERR_UNSPECIFIED;
	}
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
		CS_TYPE record_key = CS_TYPE(p_fds_evt->write.record_key);
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
//		fds_gc(); // TODO: why would we gc every time? It only leads to more flash erases.
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

