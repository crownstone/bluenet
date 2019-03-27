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

	/**
	 * Read from persistent storage.
	 *
	 * Checks if given size matches stored size, else it returns ERR_WRONG_PAYLOAD_LENGTH.
	 *
	 * @param[in] file_id         File id to read from.
	 * @param[in,out] data        Data struct with type to read. Pointer and size will be set afterwards.
	 *
	 * @retval ERR_SUCCESS                  When successful.
	 * @retval ERR_NOT_FOUND                When the type was not found.
	 * @retval ERR_WRONG_PAYLOAD_LENGTH     When the given size does not match the stored size.
	 */
	cs_ret_code_t read(cs_file_id_t file_id, cs_state_data_t & data);

	/**
	 * Write to persistent storage.
	 *
	 * It is assumed that data pointer was allocated by this class.
	 *
	 * @param[in] file_id         File id to write to.
	 * @param[in] data            Data struct with type, data pointer, and size.
	 */
	cs_ret_code_t write(cs_file_id_t file_id, const cs_state_data_t & data);

	/**
	 * Allocate ram that is correctly aligned and padded.
	 *
	 * @param[in,out] size        Requested size, afterwards set to the allocated size.
	 * @return                    Pointer to allocated memory.
	 */
	uint8_t* allocate(size16_t& size);

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

	// Returns size after padding for flash.
	size16_t getPaddedSize(size16_t size);

	/** Write to persistent storage.
	*/
	ret_code_t writeInternal(cs_file_id_t file_id, const cs_state_data_t & data);

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

