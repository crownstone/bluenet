/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <events/cs_EventListener.h>
#include <components/libraries/fds/fds.h>
#include <string>
#include <util/cs_Utils.h>

extern "C" {
	// Define event handler for errors during initialization
	static void fds_evt_handler(fds_evt_t const * p_fds_evt) {
		switch (p_fds_evt->id) {
			case FDS_EVT_INIT:
				if (p_fds_evt->result != FDS_SUCCESS) {
					// TODO: indicate that initialization failed.
				}
				break;
			default:
				break;
		}
	}
}

/** Class to store items persistently in flash (persistent) memory.
 *
 * This class provides functions to initialize, clear, write and read persistent memory (flash) through the use of 
 * Flash Data Storage.
 *
 * The information on the Flash Data Storage from Nordic can be found at the link:
 *   https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk%2Fdita%2Fsdk%2Fnrf5_sdk.html
 */
class Storage : EventListener {
public:

	typedef uint16_t file_id_t;
	typedef uint16_t key_t;
	
	static const file_id_t FILE_STATE   = 0x0000;
	static const file_id_t FILE_GENERAL = 0x0001;

	/** Returns the singleton instance of this class
	 *
	 * @return static instance of Storage class
	 */
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	ret_code_t init() {
		// clear fds token before first use 
		memset(&ftok, 0x00, sizeof(fds_find_token_t));

		// register and initialize module
		ret_code_t ret_code;
		ret_code = fds_register(fds_evt_handler);
		if (ret_code != FDS_SUCCESS) {
			// TODO: Registering of the FDS event handler has failed.
		}
		ret_code = fds_init();
		if (ret_code != FDS_SUCCESS) {
			// TODO: Handle error.
		}
		return ret_code;
	}

	bool isInitialized() {
		return _initialized;
	}

	ret_code_t write(file_id_t file_id, key_t key, const uint8_t *record_value, const uint8_t record_length) {
		fds_record_t        record;
		fds_record_desc_t   record_desc;

		record.file_id           = file_id;
		record.key               = key;
		record.data.p_data       = record_value;
		record.data.length_words = record_length;

		ret_code_t ret_code;
		ret_code = fds_record_write(&record_desc, &record);
		return ret_code;
	}

	ret_code_t read(file_id_t file_id, key_t key, uint8_t *record_value, uint8_t & record_length) {
		fds_flash_record_t flash_record;
		fds_record_desc_t  record_desc;
		ret_code_t         ret_code;

		ret_code = FDS_ERR_NOT_FOUND;

		// go through all records with given file_id and key (can be multiple)
		while (fds_record_find(file_id, key, &record_desc, &ftok) == FDS_SUCCESS) {
			ret_code = fds_record_open(&record_desc, &flash_record);
		    if (ret_code != FDS_SUCCESS) break;
		
			// map flash_record.p_data to value
			record_length = flash_record.p_header->length_words;
			memcpy(record_value, flash_record.p_data, record_length);
		
			// invalidates the record	
			ret_code = fds_record_close(&record_desc);
		    if (ret_code != FDS_SUCCESS) break;
		}
		return ret_code;
	}

	ret_code_t remove(file_id_t file_id) {
		ret_code_t        ret_code;
		ret_code = fds_file_delete(file_id);
		return ret_code;
	}

	ret_code_t remove(file_id_t file_id, key_t key) {
		fds_record_desc_t record_desc;
		ret_code_t        ret_code;

		ret_code = FDS_ERR_NOT_FOUND;

		// go through all records with given file_id and key (can be multiple)
		while (fds_record_find(file_id, key, &record_desc, &ftok) == FDS_SUCCESS) {
			ret_code = fds_record_delete(&record_desc);
		    if (ret_code != FDS_SUCCESS) break;
		}
		return ret_code;
	}

	void handleEvent(uint16_t evt, void* p_data, uint16_t length) {
		// should handle event
	}

private:
	Storage();
	Storage(Storage const&);
	void operator=(Storage const &);

	bool _initialized;

	fds_find_token_t ftok;
};

