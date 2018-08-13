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
#include <vector>

typedef uint16_t st_file_id_t;
typedef uint16_t st_key_t;

typedef struct { 
	st_key_t key;
	uint8_t *value;
	uint16_t size;
	bool persistent;
} st_file_data_t;

typedef union {
	int8_t    s8;
	int16_t   s16;
	int32_t   s32;
	uint8_t   u8;
	uint16_t  u16;
	uint32_t  u32;
} __ALIGN(4) st_value_t;

static const st_file_id_t FILE_DO_NOT_USE     = 0x0000;
static const st_file_id_t FILE_STATE          = 0x0001;
static const st_file_id_t FILE_GENERAL        = 0x0002;
static const st_file_id_t FILE_CONFIGURATION  = 0x0003;

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

	/** Returns the singleton instance of this class
	 *
	 * @return static instance of Storage class
	 */
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	ret_code_t init();
	
	inline bool isInitialized() {
		return _initialized;
	}

	/** Write to persistent storage.
	*/
	ret_code_t write(st_file_id_t file_id, st_file_data_t file_contents);

	/** Read from persistent storage.
	 */
	ret_code_t read(st_file_id_t file_id, st_file_data_t file_contents);

	ret_code_t remove(st_file_id_t file_id);

	ret_code_t remove(st_file_id_t file_id, st_key_t key);
	
	ret_code_t exists(st_file_id_t file_id);

	ret_code_t exists(st_file_id_t file_id, st_key_t key);

	ret_code_t exists(st_file_id_t file_id, st_key_t key, fds_record_desc_t record_fd);

	// Handle Crownstone events
	void handleEvent(uint16_t evt, void* p_data, uint16_t size) {
	};

	void handleFileStorageEvent(fds_evt_t const * p_fds_evt);
	
	ret_code_t garbageCollect();

private:
	Storage();
	Storage(Storage const&);
	void operator=(Storage const &);

	bool _initialized;

	fds_find_token_t _ftok;

	std::vector<st_file_data_t> _records_in_memory;	

};

