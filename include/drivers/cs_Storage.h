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
	typedef struct { 
		key_t key;
		uint8_t *value;
		uint16_t size;
		bool persistent;
	} file_data_t;
	
	// Keep the file identifiers in the Storage class itself
	static const file_id_t FILE_STATE          = 0x0000;
	static const file_id_t FILE_GENERAL        = 0x0001;
	static const file_id_t FILE_CONFIGURATION  = 0x0002;

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
	ret_code_t write(file_id_t file_id, file_data_t file_contents);

	/** Read from persistent storage.
	 */
	ret_code_t read(file_id_t file_id, file_data_t file_contents);

	ret_code_t remove(file_id_t file_id);

	ret_code_t remove(file_id_t file_id, key_t key);
	
	ret_code_t exists(file_id_t file_id);

	ret_code_t exists(file_id_t file_id, key_t key);

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

private:
	Storage();
	Storage(Storage const&);
	void operator=(Storage const &);

	bool _initialized;

	fds_find_token_t _ftok;

	std::vector<file_data_t> _records_in_memory;	

};

