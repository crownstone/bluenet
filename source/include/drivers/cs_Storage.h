/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <common/cs_Types.h>
#include <components/libraries/fds/fds.h>
#include <storage/cs_StateData.h>
#include <util/cs_Utils.h>

#include <string>
#include <vector>


enum cs_storage_operation_t {
	CS_STORAGE_OP_READ,
	CS_STORAGE_OP_WRITE,
	CS_STORAGE_OP_REMOVE,
	CS_STORAGE_OP_REMOVE_ALL_VALUES_WITH_ID,
	CS_STORAGE_OP_GC,
};

typedef void (*cs_storage_error_callback_t) (cs_storage_operation_t operation, CS_TYPE type, cs_state_id_t id);

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
class Storage {
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
	 * Find first id of stored values of given type.
	 *
	 * NOTE: you should complete or abort this findFirst() / findNext() before starting a new one.
	 *
	 * @param[in] type            Type to search for.
	 * @param[out] id             ID of the found value.
	 *
	 * @retval ERR_SUCCESS                  When successful.
	 * @retval ERR_NOT_FOUND                When the first id of this type was not found.
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t findFirst(CS_TYPE type, cs_state_id_t & id);

	/**
	 * Find next id of stored values of given type.
	 *
	 * NOTE: must be called immediately after findFirst(), no other storage operations should be done in between.
	 *
	 * @param[in] type            Type to search for.
	 * @param[out] id             ID of the found value.
	 *
	 * @retval ERR_SUCCESS                  When successful.
	 * @retval ERR_NOT_FOUND                When the next id of this type was not found.
	 * @retval ERR_WRONG_STATE              When not called after findFirst(), or another storage operation was done in between.
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t findNext(CS_TYPE type, cs_state_id_t & id);

	/**
	 * Find and read stored value of given type and id.
	 *
	 * In case of duplicates (values with same type and id), you will get the latest value.
	 *
	 * @param[in,out] data        Data struct with type and id to read. Pointer and size will be set afterwards.
	 *
	 * @retval ERR_SUCCESS                  When successful.
	 * @retval ERR_NOT_FOUND                When the type was not found.
	 * @retval ERR_WRONG_PAYLOAD_LENGTH     When the given size does not match the stored size.
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t read(cs_state_data_t & data);

	/**
	 * Read the old (v3) reset counter.
	 *
	 * Made to transfer reset counter from old location to new.
	 *
	 * @param[in, out] data       Data struct with type and id to read. Pointer and size will be set afterwards.
	 *
	 * @return Error code like read().
	 */
	cs_ret_code_t readV3ResetCounter(cs_state_data_t & data);

	/**
	 * Find and read from first stored value of given type.
	 *
	 * NOTE: you should complete or abort this readFirst() / readNext() before starting a new one.
	 *
	 * @param[in,out] data        Data struct with type to read. ID, pointer, and size will be set afterwards.
	 *
	 * @retval ERR_SUCCESS                  When successful.
	 * @retval ERR_NOT_FOUND                When the type was not found.
	 * @retval ERR_WRONG_PAYLOAD_LENGTH     When the given size does not match the stored size.
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t readFirst(cs_state_data_t & data);

	/**
	 * Find and read next stored value of given type.
	 *
	 * NOTE: must be called immediately after readFirst(), no other storage operations should be done in between.
	 *
	 * When iterating: just keep overwriting, so in case of duplicates (same type and id), you end up with the latest value.
	 *
	 * @param[in,out] data        Data struct with type to read. ID, pointer, and size will be set afterwards.
	 *
	 * @retval ERR_SUCCESS                  When successful.
	 * @retval ERR_NOT_FOUND                When the type was not found.
	 * @retval ERR_WRONG_PAYLOAD_LENGTH     When the given size does not match the stored size.
	 * @retval ERR_BUSY                     When busy, try again later.
	 */
	cs_ret_code_t readNext(cs_state_data_t & data);

	/**
	 * Write to persistent storage.
	 *
	 * It is assumed that data pointer was allocated by this class.
	 *
	 * Automatically starts garbage collection when needed.
	 *
	 * @param[in] data            Data struct with type, data pointer, and size.
	 *
	 * @retval ERR_SUCCESS                  When successfully started to write.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NO_SPACE                 When there is no space, not even after garbage collection.
	 */
	cs_ret_code_t write(const cs_state_data_t & data);

	/**
	 * Remove value of given type and id.
	 *
	 * @param[in] type            Type to remove.
	 * @param[in] id              ID of value to remove.
	 *
	 * @retval ERR_SUCCESS                  When successfully started removing the value.
	 * @retval ERR_NOT_FOUND                When no match was found, consider this a success, but don't wait for an event.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NOT_INITIALIZED          When storage hasn't been initialized yet.
	 */
	cs_ret_code_t remove(CS_TYPE type, cs_state_id_t id);

	/**
	 * Remove all values of a type.
	 *
	 * @param[in] type            Type to remove.
	 *
	 * @retval ERR_SUCCESS                  When successfully started removing the type.
	 * @retval ERR_NOT_FOUND                When no match was not found, consider this a success, but don't wait for an event.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NOT_INITIALIZED          When storage hasn't been initialized yet.
	 */
	cs_ret_code_t remove(CS_TYPE type);

	/**
	 * Remove all values with given id.
	 *
	 * @param[in] id              ID of the values to remove.
	 *
	 * @retval ERR_SUCCESS                  When successfully started removing.
	 * @retval ERR_NOT_FOUND                When no match was not found, consider this a success, but don't wait for an event.
	 * @retval ERR_BUSY                     When busy, try again later.
	 * @retval ERR_NOT_INITIALIZED          When storage hasn't been initialized yet.
	 */
	cs_ret_code_t remove(cs_state_id_t id);

	/**
	 * Perform factory reset.
	 *
	 * Make sure to restart factory reset on any error callback.
	 *
	 * @retval ERR_WAIT_FOR_SUCCESS    Successfully started removing a record.
	 * @retval ERR_SUCCESS             All records have been removed.
	 * @retval ERR_BUSY                Busy, try again later.
	 * @retval other                   Other errors, maybe retry again later?
	 */
	cs_ret_code_t factoryReset();

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
	 * Erase flash pages.
	 *
	 * Must be called before storage is initialized.
	 *
	 * @param[in] doneEvent                 Type of event that should be sent when pages have been erased.
	 * @param[in] startAddress              Address of first page to erase.
	 * @param[in] endAddress                Address of first page __not__ to erase.
	 *
	 * @retval ERR_SUCCESS                  When successfully started erasing all pages.
	 * @retval ERR_NOT_AVAILABLE            When you can't use this function (storage initialized already).
	 */
	cs_ret_code_t erasePages(const CS_TYPE doneEvent, void * startAddress, void * endAddress);

	/**
	 * Allocate ram that is correctly aligned and padded.
	 *
	 * @param[in,out] size        Requested size, afterwards set to the allocated size.
	 * @return                    Pointer to allocated memory.
	 */
	uint8_t* allocate(size16_t& size);

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

	fds_find_token_t _findToken;
	CS_TYPE _currentSearchType = CS_TYPE::CONFIG_DO_NOT_USE;

	bool _collectingGarbage = false;
	bool _removingFile = false;
	bool _performingFactoryReset = false;
	std::vector<uint16_t> _busyRecordKeys;

	/**
	 * Next page to erase. Used by eraseAllPages().
	 */
	uint32_t _erasePage = 0;

	/**
	 * Page that should _not_ be erased. Used by eraseAllPages().
	 */
	uint32_t _eraseEndPage = 0;

	CS_TYPE _eraseDoneEvent = CS_TYPE::CONFIG_DO_NOT_USE;

	/**
	 * Find next fileId for given recordKey.
	 */
	cs_ret_code_t findNextInternal(uint16_t recordKey, uint16_t & fileId);

	/**
	 * Read next fileId for given recordKey.
	 */
	cs_ret_code_t readNextInternal(uint16_t recordKey, uint16_t & fileId, uint8_t* buf, uint16_t size);

	/**
	 * Read a record: copy data to buffer, and sets fileId.
	 *
	 * Only returns success when data has been copied to buffer.
	 */
	cs_ret_code_t readRecord(fds_record_desc_t recordDesc, uint8_t* buf, uint16_t size, uint16_t & fileId);

	/** Write to persistent storage.
	*/
	ret_code_t writeInternal(const cs_state_data_t & data);

	ret_code_t garbageCollectInternal();

	bool isErasingPages();

	/**
	 * Erase next page, started via eraseAllPages().
	 */
	void eraseNextPage();

	/**
	 * Continue the factory reset process.
	 *
	 * @retval ERR_SUCCESS             Successfully started factory reset process.
	 * @retval ERR_BUSY                Busy, restart the whole factory reset from the beginning.
	 * @retval other                   Other errors.
	 */
	cs_ret_code_t continueFactoryReset();

	/**
	 * Returns size after padding for flash.
	 */
	size16_t getPaddedSize(size16_t size);

	/**
	 * Get file id, given state value id.
	 */
	uint16_t getFileId(cs_state_id_t valueId);

	/**
	 * Get state value id, given file id.
	 */
	cs_state_id_t getStateId(uint16_t fileId);

	bool isValidRecordKey(uint16_t recordKey);
	bool isValidFileId(uint16_t fileId);

	/**
	 * Start a new search, where the user wants to iterate over a certain type.
	 */
	void initSearch(CS_TYPE type);

	/**
	 * Start a new search.
	 *
	 * Call before using _findToken
	 */
	void initSearch();

//	ret_code_t exists(cs_file_id_t fileId, uint16_t recordKey, bool & result);

	/**
	 * Check if a type of record exists and return the record descriptor.
	 *
	 * @param[in] file_id                        File id to search.
	 * @param[in] recordKey                      Record key to search for.
	 * @param[out] record_desc                   Record descriptor, used to manipulate records.
	 * @param[out] result                        True if record was found.
	 */
	ret_code_t exists(cs_file_id_t file_id, uint16_t recordKey, fds_record_desc_t & record_desc, bool & result);

	void setBusy(uint16_t recordKey);
	void clearBusy(uint16_t recordKey);
	bool isBusy(uint16_t recordKey);
	bool isBusy();

	cs_ret_code_t getErrorCode(ret_code_t code);

	void handleWriteEvent(fds_evt_t const * p_fds_evt);
	void handleRemoveRecordEvent(fds_evt_t const * p_fds_evt);
	void handleRemoveFileEvent(fds_evt_t const * p_fds_evt);
	void handleGarbageCollectionEvent(fds_evt_t const * p_fds_evt);

//	inline void print(const std::string & prefix, CS_TYPE type);
};

