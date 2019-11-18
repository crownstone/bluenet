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
#include <storage/cs_StateData.h>
#include <util/cs_Utils.h>

#include <string>
#include <vector>


enum cs_storage_operation_t {
	CS_STORAGE_OP_READ,
	CS_STORAGE_OP_WRITE,
	CS_STORAGE_OP_REMOVE,
	CS_STORAGE_OP_REMOVE_FILE,
	CS_STORAGE_OP_GC,
};

typedef void (*cs_storage_error_callback_t) (cs_storage_operation_t operation, cs_file_id_t fileId, CS_TYPE type);

/**
 * Class to store items persistently in flash (persistent) memory.
 *
 * This class provides functions to initialize, clear, write and read persistent memory (flash) through the use of
 * Flash Data Storage.
 *
 * The information on the Flash Data Storage from Nordic can be found at the link:
 *   https://www.nordicsemi.com/DocLib/Content/SDK_Doc/nRF5_SDK/v15-2-0/lib_fds
 *   old: https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk%2Fdita%2Fsdk%2Fnrf5_sdk.html
 *
 * CS_TYPE is used as record key.
 *
 * Garbage collection will be automatically started by this class when needed.
 *
 * When a record is corrupted, most likely in case of power loss while writing, the CRC check will fail when opening a
 * file. In this case the record will be deleted (TODO).
 *
 * Only one record for each record key should exist. In case there are multiple (duplicates), they will be removed
 * (TODO). Since FDS always appends records, it is assumed that the last valid record should be kept. Checking for
 * duplicates is done for each write and each read.
 *
 * Some operations will block other operations. For example, you can't write a record while performing garbage
 * collection. You can't write a record while it's already being written. This is what the "busy" functions are for.
 * Each type can be set busy multiple times, for example in case multiple records of the same type are being deleted.
 *
 * FDS internally uses events NRF_EVT_FLASH_OPERATION_SUCCESS and NRF_EVT_FLASH_OPERATION_ERROR.
 * These events don't give any context of the operation, so it is not adviced to use FDS together with something else
 * that writes to flash (like Flash Manager).
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
	 * Set the callback for errors.
	 *
	 * These are errors like timeouts, that come in after a write/read/remove function returned.
	 *
	 * @param[in] callback                  Function to be called when an error occurred.
	 */
	void setErrorCallback(cs_storage_error_callback_t callback);

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
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t read(cs_file_id_t file_id, cs_state_data_t & data);

	/**
	 * Write to persistent storage.
	 *
	 * It is assumed that data pointer was allocated by this class.
	 *
	 * Automatically starts garbage collection when needed.
	 *
	 * @param[in] file_id         File id to write to.
	 * @param[in] data            Data struct with type, data pointer, and size.
	 *
	 * @retval ERR_SUCCESS                  When successfully started to write.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NO_SPACE                 When there is no space, not even after garbage collection.
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
	 * Removes a whole file.
	 *
	 * TODO: test this function
	 *
	 * @param[in] file_id         File id to remove.
	 *
	 * @retval ERR_SUCCESS                  When successfully started removing the file.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NOT_INITIALIZED          When storage hasn't been initialized yet.
	 */
	cs_ret_code_t remove(cs_file_id_t file_id);

	/**
	 * Removes a type from a file.
	 *
	 * TODO: test this function
	 *
	 * @param[in] file_id         File id to remove type from.
	 * @param[in] type            Type to remove
	 *
	 * @retval ERR_SUCCESS                  When successfully started removing the type.
	 * @retval ERR_NOT_FOUND                When type was not found on file, consider this a success, but don't wait for an event.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NOT_INITIALIZED          When storage hasn't been initialized yet.
	 */
	cs_ret_code_t remove(cs_file_id_t file_id, CS_TYPE type);

	/**
	 * Garbage collection reclaims the flash space that is occupied by records that have been deleted,
	 * or that failed to be completely written due to, for example, a power loss.
	 *
	 * @retval ERR_SUCCESS                  When successfully started garbage collection.
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t garbageCollect();

	/**
	 * Erase all flash pages used by FDS.
	 *
	 * This should only be used to recover from a failing init.
	 *
	 * @retval ERR_SUCCESS                  When successfully started erasing all pages.
	 * @retval ERR_NOT_AVAILABLE            When you can't use this function (storage initialized already).
	 */
	cs_ret_code_t eraseAllPages();

	/**
	 * Handle Crownstone events.
	 */
	void handleEvent(event_t &) {};

	/**
	 * Handle FDS events.
	 */
	void handleFileStorageEvent(fds_evt_t const * p_fds_evt);

	/**
	 * Handle FDS SOC event NRF_EVT_FLASH_OPERATION_SUCCESS.
	 */
	void handleFlashOperationSuccess();

	/**
	 * Handle FDS SOC event NRF_EVT_FLASH_OPERATION_ERROR.
	 */
	void handleFlashOperationError();

private:
	Storage();
	Storage(Storage const&);
	void operator=(Storage const &);

	bool _initialized = false;
	bool _registeredFds = false;
	cs_storage_error_callback_t _errorCallback = NULL;

	bool _collectingGarbage = false;

	fds_find_token_t _ftok;

	bool _removingFile = false;
	std::vector<uint16_t> _busy_record_keys;

	/**
	 * Next page to erase.
	 *
	 * Used by eraseAllPages().
	 */
	uint32_t _erasePage = 0;
	/**
	 * Page that should _not_ be erased.
	 *
	 * Used by eraseAllPages().
	 */
	uint32_t _eraseEndPage = 0;

	bool isErasingPages();

	/**
	 * Erase next page, started via eraseAllPages().
	 */
	void eraseNextPage();

	// Use before ftok
	void initSearch();

	void setBusy(uint16_t recordKey);
	void clearBusy(uint16_t recordKey);
	bool isBusy(uint16_t recordKey);
	bool isBusy();

	cs_ret_code_t getErrorCode(ret_code_t code);

	// Returns size after padding for flash.
	size16_t getPaddedSize(size16_t size);

	/** Write to persistent storage.
	*/
	ret_code_t writeInternal(cs_file_id_t file_id, const cs_state_data_t & data);

	ret_code_t exists(cs_file_id_t file_id, uint16_t recordKey, bool & result);

	/**
	 * Check if a type of record exists and return the record descriptor.
	 *
	 * @param[in] file_id                        File id to search.
	 * @param[in] recordKey                      Record key to search for.
	 * @param[out] record_desc                   Record descriptor, used to manipulate records.
	 * @param[out] result                        True if record was found.
	 */
	ret_code_t exists(cs_file_id_t file_id, uint16_t recordKey, fds_record_desc_t & record_desc, bool & result);

	ret_code_t garbageCollectInternal();

	void handleWriteEvent(fds_evt_t const * p_fds_evt);
	void handleRemoveRecordEvent(fds_evt_t const * p_fds_evt);
	void handleRemoveFileEvent(fds_evt_t const * p_fds_evt);
	void handleGarbageCollectionEvent(fds_evt_t const * p_fds_evt);

	inline void print(const std::string & prefix, CS_TYPE type);
};

