/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <components/libraries/fds/fds.h>
#include <string>
#include <util/cs_Utils.h>
#include <vector>


static const cs_file_id_t FILE_DO_NOT_USE     = 0x0000;
static const cs_file_id_t FILE_STATE          = 0x0001;
static const cs_file_id_t FILE_GENERAL        = 0x0002;
static const cs_file_id_t FILE_CONFIGURATION  = 0x0003;

/** Class to store items persistently in flash (persistent) memory.
 *
 * This class provides functions to initialize, clear, write and read persistent memory (flash) through the use of
 * Flash Data Storage.
 *
 * CS_TYPE is used as record key.
 *
 * The information on the Flash Data Storage from Nordic can be found at the link:
 *   https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk%2Fdita%2Fsdk%2Fnrf5_sdk.html
 */
class Storage : public EventListener {
public:

	/** Returns the singleton instance of this class
	 *
	 * @return static instance of Storage class
	 */
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	cs_ret_code_t init();

	inline bool isInitialized() {
		return _initialized;
	}

	/** Read from persistent storage.
	 */
	cs_ret_code_t read(cs_file_id_t file_id, cs_state_data_t data);

	/** Write to persistent storage.
	*/
	cs_ret_code_t write(cs_file_id_t file_id, cs_state_data_t data);

	/**
	 * Removes a whole flash page.
	 */
	cs_ret_code_t remove(cs_file_id_t file_id);

	/**
	 * Removes a type from a flash page.
	 */
	cs_ret_code_t remove(cs_file_id_t file_id, CS_TYPE type);

	/**
	 * Garbage collection reclaims the flash space that is occupied by records that have been deleted,
	 * or that failed to be completely written due to, for example, a power loss.
	 *
	 * This function is asynchronous.
	 */
	cs_ret_code_t garbageCollect();

	// Handle Crownstone events
	void handleEvent(event_t & event) {};

	void handleFileStorageEvent(fds_evt_t const * p_fds_evt);

	void handleSuccessfulEvent(fds_evt_t const * p_fds_evt);

private:
	Storage();
	Storage(Storage const&);
	void operator=(Storage const &);

	bool _initialized;

	fds_find_token_t _ftok;

	std::vector<cs_state_data_t> _records_in_memory;

	// Use before ftok
	void initSearch();

	cs_ret_code_t getErrorCode(ret_code_t code);






	/** Write to persistent storage.
	*/
	ret_code_t writeInternal(cs_file_id_t file_id, cs_state_data_t file_contents);

//	ret_code_t exists(cs_file_id_t file_id, bool & result);

	ret_code_t exists(cs_file_id_t file_id, CS_TYPE type, bool & result);

	/** Check if a type of record exists and return the record descriptor.
	 *
	 * @param[in] file_id                        Unique file
	 * @param[in] type                           One of the Crownstone types
	 * @param[in,out] record_desc                Record descriptor
	 * @param[out] result                        True if descriptor exists.
	 */
	ret_code_t exists(cs_file_id_t file_id, CS_TYPE type, fds_record_desc_t & record_desc, bool & result);

	ret_code_t garbageCollectInternal();

	inline void print(const std::string & prefix, CS_TYPE type);
};

