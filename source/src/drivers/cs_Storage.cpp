/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <climits>
#include <common/cs_Handlers.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <float.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <util/cs_BleError.h>
//#include <algorithm>

#define LOGStorageInit LOGi
#define LOGStorageInfo LOGi
#define LOGStorageWrite LOGd
#define LOGStorageDebug LOGvv
#define LOGStorageVerbose LOGvv

Storage::Storage() {}

cs_ret_code_t Storage::init() {
	LOGStorageInit("Init storage");
	ret_code_t fds_ret_code;

	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}

	if (_initialized) {
		LOGStorageInit("Already initialized");
		return ERR_SUCCESS;
	}

	if (!_registeredFds) {
		LOGStorageVerbose("fds_register");
		fds_ret_code = fds_register(fds_evt_handler);
		if (fds_ret_code != NRF_SUCCESS) {
			LOGe("Registering FDS event handler failed (err=%i)", fds_ret_code);
			return getErrorCode(fds_ret_code);
		}
		_registeredFds = true;
	}

	LOGStorageVerbose("fds_init");
	fds_ret_code = fds_init();
	if (fds_ret_code != NRF_SUCCESS) {
		LOGe("Init FDS failed (err=%i)", fds_ret_code);
		return getErrorCode(fds_ret_code);
	}

	LOGStorageInit("Storage init success, wait for event.");
	return getErrorCode(fds_ret_code);
}

void Storage::setErrorCallback(cs_storage_error_callback_t callback) {
	_errorCallback = callback;
}

cs_ret_code_t Storage::findFirst(CS_TYPE type, cs_state_id_t & id) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(type);
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	initSearch(type);
	uint16_t fileId;
	cs_ret_code_t retVal = findNextInternal(recordKey, fileId);
	if (retVal == ERR_SUCCESS) {
		id = getStateId(fileId);
	}
	return retVal;
}

cs_ret_code_t Storage::findNext(CS_TYPE type, cs_state_id_t & id) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(type);
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	if (_currentSearchType != type) {
		return ERR_WRONG_STATE;
	}
	uint16_t fileId;
	cs_ret_code_t retVal = findNextInternal(recordKey, fileId);
	if (retVal == ERR_SUCCESS) {
		id = getStateId(fileId);
	}
	return retVal;
}

cs_ret_code_t Storage::findNextInternal(uint16_t recordKey, uint16_t & fileId) {
	if (!isValidRecordKey(recordKey)) {
		return ERR_WRONG_PARAMETER;
	}
	fds_record_desc_t recordDesc;
	fds_flash_record_t flashRecord;
	ret_code_t fdsRetCode;
	while (fds_record_find_by_key(recordKey, &recordDesc, &_findToken) == NRF_SUCCESS) {
		fdsRetCode = fds_record_open(&recordDesc, &flashRecord);
		if (fdsRetCode == NRF_SUCCESS) {
			fileId = flashRecord.p_header->file_id;
			fds_record_close(&recordDesc);
			return ERR_SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}



/**
 * Iterate over all records, so that in case of duplicates, the last written record will be used to read.
 */
cs_ret_code_t Storage::read(cs_state_data_t & stateData) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	uint16_t fileId = getFileId(stateData.id);
	if (!isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	fds_record_desc_t recordDesc;
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;
	bool done = false;
	LOGStorageDebug("Read record key=%u file=%u", recordKey, fileId);
	initSearch();
	while (fds_record_find(fileId, recordKey, &recordDesc, &_findToken) == NRF_SUCCESS) {
		if (done) {
			LOGe("Duplicate record key=%u file=%u addr=%p", recordKey, fileId, _findToken.p_addr);
		}
		csRetCode = readRecord(recordDesc, stateData.value, stateData.size, fileId);
		if (csRetCode == ERR_SUCCESS) {
			done = true;
		}
//		if (done) {
//			break;
//		}
	}
	if (done) {
		return ERR_SUCCESS;
	}
	if (csRetCode == ERR_NOT_FOUND) {
		LOGStorageDebug("Record not found");
	}
	return csRetCode;
}

cs_ret_code_t Storage::readV3ResetCounter(cs_state_data_t & stateData) {
	// Code copied from read() with only fileId changed.
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	uint16_t fileId = FILE_KEEP_FOREVER;
	if (stateData.type != CS_TYPE::STATE_RESET_COUNTER || stateData.id != 0 || !isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	fds_record_desc_t recordDesc;
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;
	bool done = false;
	LOGStorageDebug("Read record %u file=%u", recordKey, fileId);
	initSearch();
	while (fds_record_find(fileId, recordKey, &recordDesc, &_findToken) == NRF_SUCCESS) {
		if (done) {
			LOGe("Duplicate record key=%u file=%u addr=%p", recordKey, fileId, _findToken.p_addr);
		}
		csRetCode = readRecord(recordDesc, stateData.value, stateData.size, fileId);
		if (csRetCode == ERR_SUCCESS) {
			done = true;
		}
//		if (done) {
//			break;
//		}
	}
	if (done) {
		return ERR_SUCCESS;
	}
	if (csRetCode == ERR_NOT_FOUND) {
		LOGStorageDebug("Record not found");
	}
	return csRetCode;
}

cs_ret_code_t Storage::readFirst(cs_state_data_t & stateData) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	initSearch(stateData.type);
	uint16_t fileId;
	cs_ret_code_t retVal = readNextInternal(recordKey, fileId, stateData.value, stateData.size);
	if (retVal == ERR_SUCCESS) {
		stateData.id = getStateId(fileId);
	}
	return retVal;
}


cs_ret_code_t Storage::readNext(cs_state_data_t & stateData) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	if (_currentSearchType != stateData.type) {
		return ERR_WRONG_STATE;
	}
	uint16_t fileId;
	cs_ret_code_t retVal = readNextInternal(recordKey, fileId, stateData.value, stateData.size);
	if (retVal == ERR_SUCCESS) {
		stateData.id = getStateId(fileId);
	}
	return retVal;
}

/**
 * Simply reads the next valid record of given key.
 */
cs_ret_code_t Storage::readNextInternal(uint16_t recordKey, uint16_t & fileId, uint8_t* buf, uint16_t size) {
	if (!isValidRecordKey(recordKey)) {
		return ERR_WRONG_PARAMETER;
	}
	fds_record_desc_t recordDesc;
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;
	while (fds_record_find_by_key(recordKey, &recordDesc, &_findToken) == NRF_SUCCESS) {
		csRetCode = readRecord(recordDesc, buf, size, fileId);
		if (csRetCode == ERR_SUCCESS) {
			return csRetCode;
		}
	}
	return csRetCode;
}

cs_ret_code_t Storage::readRecord(fds_record_desc_t recordDesc, uint8_t* buf, uint16_t size, uint16_t & fileId) {
	fds_flash_record_t flashRecord;
	ret_code_t fdsRetCode = fds_record_open(&recordDesc, &flashRecord);
	switch (fdsRetCode) {
	case NRF_SUCCESS: {
		cs_ret_code_t csRetCode = ERR_SUCCESS;
		LOGStorageVerbose("Opened record id=%u addr=%p", flashRecord.p_header->record_id, recordDesc.p_record);
		size_t flashSize = flashRecord.p_header->length_words << 2; // Size is in bytes, each word is 4B.
		if (flashSize != getPaddedSize(size)) {
			LOGe("stored size = %u ram size = %u", flashSize, size);
			// TODO: remove this record?
			csRetCode = ERR_WRONG_PAYLOAD_LENGTH;
		}
		else {
			fileId = flashRecord.p_header->file_id;
			memcpy(buf, flashRecord.p_data, size);
		}
		if (fds_record_close(&recordDesc) != NRF_SUCCESS) {
			// TODO: How to handle the close error? Maybe reboot?
			LOGe("Error on closing record err=%u", fdsRetCode);
		}
		return csRetCode;
		break;
	}
	case FDS_ERR_CRC_CHECK_FAILED:
		LOGw("CRC check failed addr=%p", recordDesc.p_record);
		// TODO: remove record.
		break;
	case FDS_ERR_NOT_FOUND:
	default:
		LOGw("Unhandled open error: %u", fdsRetCode);
	}
	return getErrorCode(fdsRetCode);
}

cs_ret_code_t Storage::write(const cs_state_data_t & stateData) {
	return getErrorCode(writeInternal(stateData));
}

/**
 * When the space is exhausted, write requests return the error FDS_ERR_NO_SPACE_IN_FLASH, and you must
 * run garbage collection and wait for completion before repeating the call to the write function.
 */
ret_code_t Storage::writeInternal(const cs_state_data_t & stateData) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	uint16_t fileId = getFileId(stateData.id);
	if (!isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return FDS_ERR_BUSY;
	}
	fds_record_t record;
	fds_record_desc_t recordDesc;
	ret_code_t fdsRetCode;

	record.file_id           = fileId;
	record.key               = recordKey;
	record.data.p_data       = stateData.value;
	// Assume the allocation was done by storage.
	// Size is in bytes, each word is 4B.
	record.data.length_words = getPaddedSize(stateData.size) >> 2;
	LOGStorageWrite("Write key=%u file=%u", recordKey, fileId);
	LOGStorageVerbose("Data=%p word size=%u", record.data.p_data, record.data.length_words);

	bool recordExists = false;
	fdsRetCode = exists(fileId, recordKey, recordDesc, recordExists);
	if (recordExists) {
		LOGStorageVerbose("Update key=%u file=%u ptr=%p", record.key, record.file_id, record.data.p_data);
		fdsRetCode = fds_record_update(&recordDesc, &record);
	}
	else {
		LOGStorageVerbose("Write key=%u file=%u ptr=%p", record.key, record.file_id, record.data.p_data);
		fdsRetCode = fds_record_write(&recordDesc, &record);
	}
	switch (fdsRetCode) {
		case NRF_SUCCESS:
			setBusy(recordKey);
			LOGStorageVerbose("Started writing");
			break;
		case FDS_ERR_NO_SPACE_IN_FLASH: {
			LOGStorageInfo("Flash is full, start garbage collection");
			ret_code_t gcRetCode = garbageCollect();
			if (gcRetCode == NRF_SUCCESS) {
				fdsRetCode = FDS_ERR_BUSY;
			}
			else {
				LOGe("Failed to start GC: %u", gcRetCode);
				fdsRetCode = gcRetCode;
			}
			break;
		}
		case FDS_ERR_NO_SPACE_IN_QUEUES:
		case FDS_ERR_BUSY:
			break;
		default:
			LOGw("Unhandled write error: %u", fdsRetCode);
	}
	return fdsRetCode;
}

cs_ret_code_t Storage::remove(CS_TYPE type, cs_state_id_t id) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(type);
	uint16_t fileId = getFileId(id);
	if (!isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	LOGStorageDebug("Remove key=%u file=%u", recordKey, fileId);
	fds_record_desc_t recordDesc;
	ret_code_t fdsRetCode = FDS_ERR_NOT_FOUND;

	// Go through all records with given fileId and key (can be multiple).
	// Record key can be set busy multiple times.
	initSearch();
	while (fds_record_find(fileId, recordKey, &recordDesc, &_findToken) == NRF_SUCCESS) {
		fdsRetCode = fds_record_delete(&recordDesc);
		LOGStorageVerbose("fds_record_delete %u", fdsRetCode);
		if (fdsRetCode == NRF_SUCCESS) {
			setBusy(recordKey);
		}
		else {
			break;
		}
	}
	return getErrorCode(fdsRetCode);
}

cs_ret_code_t Storage::remove(CS_TYPE type) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(type);
	if (!isValidRecordKey(recordKey)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	LOGStorageDebug("Remove key=%u", recordKey);
	fds_record_desc_t recordDesc;
	ret_code_t fdsRetCode = FDS_ERR_NOT_FOUND;

	// Go through all records with given key (can be multiple).
	// Record key can be set busy multiple times.
	initSearch();
	while (fds_record_find_by_key(recordKey, &recordDesc, &_findToken) == NRF_SUCCESS) {
		fdsRetCode = fds_record_delete(&recordDesc);
		LOGStorageVerbose("fds_record_delete %u", fdsRetCode);
		if (fdsRetCode == NRF_SUCCESS) {
			setBusy(recordKey);
		}
		else {
			break;
		}
	}
	return getErrorCode(fdsRetCode);
}

cs_ret_code_t Storage::remove(cs_state_id_t id) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t fileId = getFileId(id);
	if (!isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy()) {
		return ERR_BUSY;
	}
	ret_code_t fdsRetCode = fds_file_delete(fileId);
	if (fdsRetCode == NRF_SUCCESS) {
		_removingFile = true;
	}
	return getErrorCode(fdsRetCode);
}

cs_ret_code_t Storage::factoryReset() {
	LOGStorageInfo("factoryReset");
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	if (isBusy()) {
		return ERR_BUSY;
	}
	initSearch();
	cs_ret_code_t retCode = continueFactoryReset();
	if (retCode == ERR_SUCCESS) {
		_performingFactoryReset = true;
	}
	return retCode;
}

cs_ret_code_t Storage::continueFactoryReset() {
	LOGStorageVerbose("continueFactoryReset");
	fds_record_desc_t recordDesc;
	fds_flash_record_t flashRecord;
	ret_code_t fdsRetCode ;
	while (fds_record_iterate(&recordDesc, &_findToken) == NRF_SUCCESS) {
		bool remove = false;
		fdsRetCode = fds_record_open(&recordDesc, &flashRecord);
		switch (fdsRetCode) {
			case NRF_SUCCESS: {
				uint16_t fileId = flashRecord.p_header->file_id;
				uint16_t recordKey = flashRecord.p_header->record_key;
				if (fds_record_close(&recordDesc) != NRF_SUCCESS) {
					// TODO: How to handle the close error? Maybe reboot?
					LOGe("Error on closing record");
				}
				CS_TYPE type = toCsType(recordKey);
				cs_state_id_t id = getStateId(fileId);

				remove = removeOnFactoryReset(type, id);
				if (!remove) {
					LOGStorageVerbose("skip record type=%u id=%u recordKey=%u fileId=%u", to_underlying_type(type), id, recordKey, fileId);
				}
				else {
					LOGStorageVerbose("remove record type=%u id=%u recordKey=%u fileId=%u", to_underlying_type(type), id, recordKey, fileId);
				}
				break;
			}
			case FDS_ERR_CRC_CHECK_FAILED:
				LOGStorageVerbose("remove record with crc fail");
				remove = true;
				break;
			case FDS_ERR_NOT_FOUND:
			default:
				LOGw("Unhandled open error: %u", fdsRetCode);
				return getErrorCode(fdsRetCode);
		}
		if (remove) {
			fdsRetCode = fds_record_delete(&recordDesc);
			switch (fdsRetCode) {
				case NRF_SUCCESS:
					return ERR_SUCCESS;
				case FDS_ERR_NO_SPACE_IN_QUEUES:
					return ERR_BUSY;
				default:
					return getErrorCode(fdsRetCode);
			}
		}
	}
	LOGStorageInfo("Done removing all records.");
	fdsRetCode = fds_gc();
	if (fdsRetCode != NRF_SUCCESS) {
		LOGw("Failed to start garbage collection (err=%i)", fdsRetCode);
		return getErrorCode(fdsRetCode);
	}
	else {
		LOGStorageDebug("Started garbage collection");
		_collectingGarbage = true;
		return ERR_SUCCESS;
	}
}

cs_ret_code_t Storage::garbageCollect() {
	return getErrorCode(garbageCollectInternal());
}

ret_code_t Storage::garbageCollectInternal() {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	ret_code_t fdsRetCode;
	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}
	if (isBusy()) {
		return FDS_ERR_BUSY;
	}
	LOGStorageVerbose("fds_gc");
	fdsRetCode = fds_gc();
	if (fdsRetCode != NRF_SUCCESS) {
		LOGw("Failed to start garbage collection (err=%i)", fdsRetCode);
	}
	else {
		LOGStorageDebug("Started garbage collection");
		_collectingGarbage = true;
	}
	return fdsRetCode;
}

cs_ret_code_t Storage::eraseAllPages() {
	LOGw("eraseAllPages");
	if (_initialized || isErasingPages()) {
		return ERR_NOT_AVAILABLE;
	}
	uint32_t endAddr = fds_flash_end_addr();
	uint32_t flashSizeWords = FDS_VIRTUAL_PAGES * FDS_VIRTUAL_PAGE_SIZE;
	uint32_t flashSizeBytes = flashSizeWords * sizeof(uint32_t);
	uint32_t startAddr = endAddr - flashSizeBytes;

	// Check if pages are already erased.
	unsigned int* startAddrPointer = (unsigned int*)startAddr;
	bool isErased = true;
	for (uint32_t i = 0; i < flashSizeWords; ++i) {
		if (startAddrPointer[i] != 0xFFFFFFFF) {
			isErased = false;
			break;
		}
	}
	if (isErased) {
		LOGw("Flash pages used for FDS are already erased");
		// No use to erase pages again.
		return ERR_NOT_AVAILABLE;
	}

	return erasePages(CS_TYPE::EVT_STORAGE_PAGES_ERASED, (void *)startAddr, (void *)endAddr);
}

cs_ret_code_t Storage::erasePages(const CS_TYPE doneEvent, void * startAddressPtr, void * endAddressPtr) {
	if (_initialized || isErasingPages()) {
		return ERR_NOT_AVAILABLE;
	}
	unsigned int startAddr = (unsigned int)startAddressPtr;
	unsigned int endAddr = (unsigned int)endAddressPtr;
	unsigned int const pageSize = NRF_FICR->CODEPAGESIZE;
	unsigned int startPage = startAddr / pageSize;
	unsigned int endPage = endAddr / pageSize;
	if (startPage > endPage) {
		APP_ERROR_CHECK(NRF_ERROR_INVALID_ADDR);
	}

	LOGw("Erase flash pages: [0x%p - 0x%p) or [%u - %u)", startAddr, endAddr, startPage, endPage);
	_erasePage = startPage;
	_eraseEndPage = endPage;
	_eraseDoneEvent = doneEvent;
	eraseNextPage();
	return ERR_SUCCESS;
}

bool Storage::isErasingPages() {
	return (_erasePage != 0 && _eraseEndPage != 0 && _erasePage <= _eraseEndPage);
}

void Storage::eraseNextPage() {
	if (_erasePage != 0 && _eraseEndPage != 0 && _erasePage <= _eraseEndPage) {
		if (_eraseEndPage == _erasePage) {
			LOGStorageInfo("Done erasing pages");
			event_t event(_eraseDoneEvent);
			EventDispatcher::getInstance().dispatch(event);
		}
		else {
			uint32_t result = NRF_ERROR_BUSY;
			while (result == NRF_ERROR_BUSY) {
				LOGStorageVerbose("Erase page %u", _erasePage);
				result = sd_flash_page_erase(_erasePage);
				LOGStorageInfo("Erase result: %u", result);
			}
			APP_ERROR_CHECK(result);
			_erasePage++;
			LOGStorageInfo("Wait for flash operation success");
		}
	}
}

/**
 * Maybe we should check if data is stored at right boundary.
 *
 * if ((uint32_t)data.value % 4u != 0) {
 *		LOGe("Unaligned type: %u: %p", type, data.value);
 *	}
 */
uint8_t* Storage::allocate(size16_t& size) {
	// TODO Bart 2019-12-12 Can also use new with align, according to arend.
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

/*
 * Default id = 0
 * Old configs were all at file id FILE_CONFIGURATION.
 * So for id 0, we want to get FILE_CONFIGURATION.
 */
uint16_t Storage::getFileId(cs_state_id_t valueId) {
	return valueId + FILE_CONFIGURATION;
}

cs_state_id_t Storage::getStateId(uint16_t fileId) {
	return fileId - FILE_CONFIGURATION;
}

bool Storage::isValidRecordKey(uint16_t recordKey) {
	return (recordKey > 0 && recordKey < 0xBFFF);
}

bool Storage::isValidFileId(uint16_t fileId) {
	return (fileId < 0xBFFF);
}

void Storage::initSearch(CS_TYPE type) {
	initSearch();
	_currentSearchType = type;
}

void Storage::initSearch() {
	// clear fds token before every use
	memset(&_findToken, 0x00, sizeof(fds_find_token_t));
	_currentSearchType = CS_TYPE::CONFIG_DO_NOT_USE;
}

//ret_code_t Storage::exists(cs_file_id_t file_id, uint16_t recordKey, bool & result) {
//	fds_record_desc_t record_desc;
//	return exists(file_id, recordKey, record_desc, result);
//}

/**
 * Check if a record exists.
 * Warns when it's found multiple times, but doesn't delete them. That would require to make a copy of the first found
 * record_desc.
 * Returns the last found record.
 */
ret_code_t Storage::exists(cs_file_id_t fileId, uint16_t recordKey, fds_record_desc_t & record_desc, bool & result) {
	initSearch();
	result = false;
	while (fds_record_find(fileId, recordKey, &record_desc, &_findToken) == NRF_SUCCESS) {
		if (result) {
			LOGe("Duplicate record key=%u file=%u addr=%p", recordKey, fileId, _findToken.p_addr);
//			fds_record_delete(&record_desc);
		}
		if (!result) {
			result = true;
		}
	}
	return ERR_SUCCESS;
}

void Storage::setBusy(uint16_t recordKey) {
	_busyRecordKeys.push_back(recordKey);
}

void Storage::clearBusy(uint16_t recordKey) {
	for (auto it = _busyRecordKeys.begin(); it != _busyRecordKeys.end(); it++) {
		if (*it == recordKey) {
			_busyRecordKeys.erase(it);
			return;
		}
	}
}

bool Storage::isBusy(uint16_t recordKey) {
	if (_collectingGarbage || _removingFile || _performingFactoryReset) {
		LOGw("Busy: gc=%u rm_file=%u factoryReset=%u", _collectingGarbage, _removingFile, _performingFactoryReset);
		return true;
	}
	for (auto it = _busyRecordKeys.begin(); it != _busyRecordKeys.end(); it++) {
		if (*it == recordKey) {
			LOGw("Busy with record %u", recordKey);
			return true;
		}
	}
	return false;
	// This costs us 400B or so, not really worth it..
//	return (std::find(_busy_record_keys.begin(), _busy_record_keys.end(), recordKey) != _busy_record_keys.end());
}

bool Storage::isBusy() {
	if (_collectingGarbage || _removingFile || _performingFactoryReset || !_busyRecordKeys.empty()) {
		LOGw("Busy: gc=%u rm_file=%u factoryReset=%u records=%u", _collectingGarbage, _removingFile, _performingFactoryReset, _busyRecordKeys.size());
		return true;
	}
	return false;
}

/**
 * Returns cs_ret_code_t given a ret_code_t from FDS.
 */
cs_ret_code_t Storage::getErrorCode(ret_code_t code) {
	switch (code) {
	case NRF_SUCCESS:
		return ERR_SUCCESS;
	case FDS_ERR_NOT_FOUND:
		return ERR_NOT_FOUND;
	case FDS_ERR_NOT_INITIALIZED:
		return ERR_NOT_INITIALIZED;
	case FDS_ERR_NO_SPACE_IN_FLASH:
		return ERR_NO_SPACE;
	case FDS_ERR_NO_SPACE_IN_QUEUES:
	case FDS_ERR_BUSY:
		return ERR_BUSY;
	case FDS_ERR_UNALIGNED_ADDR:
	case FDS_ERR_INVALID_ARG:
	case FDS_ERR_NULL_ARG:
		return ERR_WRONG_PARAMETER;
	default:
		return ERR_UNSPECIFIED;
	}
}

void Storage::handleWriteEvent(fds_evt_t const * p_fds_evt) {
	clearBusy(p_fds_evt->write.record_key);
	TYPIFY(EVT_STORAGE_WRITE_DONE) eventData;
	eventData.type = CS_TYPE(p_fds_evt->write.record_key);
	eventData.id = getStateId(p_fds_evt->write.file_id);
	switch (p_fds_evt->result) {
	case NRF_SUCCESS: {
		LOGStorageWrite("Write done, key=%u file=%u type=%u id=%u", p_fds_evt->del.record_key, p_fds_evt->del.file_id, to_underlying_type(eventData.type), eventData.id);
		event_t event(CS_TYPE::EVT_STORAGE_WRITE_DONE, &eventData, sizeof(eventData));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	default:
		LOGw("Write FDSerror=%u key=%u file=%u", p_fds_evt->result, p_fds_evt->write.record_key, p_fds_evt->write.file_id);
		if (_errorCallback) {
			_errorCallback(CS_STORAGE_OP_WRITE, eventData.type, eventData.id);
		}
		else {
			LOGw("Unhandled");
		}
		break;
	}
}

void Storage::handleRemoveRecordEvent(fds_evt_t const * p_fds_evt) {
	clearBusy(p_fds_evt->del.record_key);
	TYPIFY(EVT_STORAGE_REMOVE_DONE) eventData;
	eventData.type = toCsType(p_fds_evt->del.record_key);
	eventData.id = getStateId(p_fds_evt->del.file_id);
	switch (p_fds_evt->result) {
	case NRF_SUCCESS: {
		LOGStorageInfo("Remove done, key=%u file=%u type=%u id=%u", p_fds_evt->del.record_key, p_fds_evt->del.file_id, to_underlying_type(eventData.type), eventData.id);
		if (_performingFactoryReset) {
			continueFactoryReset();
		}
		else {
			event_t event(CS_TYPE::EVT_STORAGE_REMOVE_DONE, &eventData, sizeof(eventData));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	default:
		LOGw("Remove FDSerror=%u key=%u file=%u", p_fds_evt->result, p_fds_evt->del.record_key, p_fds_evt->del.file_id);
		if (_errorCallback) {
			_errorCallback(CS_STORAGE_OP_REMOVE, eventData.type, eventData.id);
		}
		else {
			LOGw("Unhandled");
		}
	}
}

void Storage::handleRemoveFileEvent(fds_evt_t const * p_fds_evt) {
	_removingFile = false;
	cs_state_id_t id = getStateId(p_fds_evt->write.file_id);
	switch (p_fds_evt->result) {
	case NRF_SUCCESS: {
		LOGStorageInfo("Remove file done, file=%u id=%u", p_fds_evt->del.file_id, id);
		event_t event(CS_TYPE::EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE, &id, sizeof(id));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	default:
		LOGw("Remove FDSerror=%u file=%u", p_fds_evt->result, p_fds_evt->del.file_id);
		if (_errorCallback) {
			_errorCallback(CS_STORAGE_OP_REMOVE_ALL_VALUES_WITH_ID, CS_TYPE::CONFIG_DO_NOT_USE, id);
		}
		else {
			LOGw("Unhandled");
		}
	}
}

void Storage::handleGarbageCollectionEvent(fds_evt_t const * p_fds_evt) {
	_collectingGarbage = false;
	switch (p_fds_evt->result) {
	case NRF_SUCCESS: {
		LOGStorageInfo("Garbage collection successful");
		if (_performingFactoryReset) {
			_performingFactoryReset = false;
			event_t resetEvent(CS_TYPE::EVT_STORAGE_FACTORY_RESET_DONE);
			EventDispatcher::getInstance().dispatch(resetEvent);
			return;
		}
		event_t event(CS_TYPE::EVT_STORAGE_GC_DONE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case FDS_ERR_OPERATION_TIMEOUT:
		LOGw("Garbage collection timeout");
	default:
		if (_errorCallback) {
			_errorCallback(CS_STORAGE_OP_GC, CS_TYPE::CONFIG_DO_NOT_USE, 0);
		}
		else {
			LOGw("Unhandled GC error: %u", p_fds_evt->result);
		}
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
	LOGStorageVerbose("FS: res=%u evt=%u", p_fds_evt->result, p_fds_evt->id);
	if (_performingFactoryReset && p_fds_evt->result != NRF_SUCCESS) {
		LOGw("Stopped factory reset process");
		_performingFactoryReset = false;
	}
	switch (p_fds_evt->id) {
		case FDS_EVT_INIT: {
			if (p_fds_evt->result == NRF_SUCCESS) {
				LOGStorageInit("Storage initialized");
				_initialized = true;
				event_t event(CS_TYPE::EVT_STORAGE_INITIALIZED);
				EventDispatcher::getInstance().dispatch(event);
			}
			else {
				LOGe("Failed to init storage: %u", p_fds_evt->result);
				// Only option left is to reboot and see if things work out next time.
				APP_ERROR_CHECK(p_fds_evt->result);
			}
			break;
		}
		case FDS_EVT_WRITE:
		case FDS_EVT_UPDATE:
			handleWriteEvent(p_fds_evt);
			break;
		case FDS_EVT_DEL_RECORD:
			handleRemoveRecordEvent(p_fds_evt);
			break;
		case FDS_EVT_DEL_FILE:
			handleRemoveFileEvent(p_fds_evt);
			break;
		case FDS_EVT_GC:
			handleGarbageCollectionEvent(p_fds_evt);
			break;
	}
}

void Storage::handleFlashOperationSuccess() {
	eraseNextPage();
}

void Storage::handleFlashOperationError() {
	if (isErasingPages()) {
		LOGw("Flash operation error");
		// Only option left is to reboot and see if things work out next time.
		// Not sure this is the correct error to throw.
		APP_ERROR_CHECK(NRF_ERROR_TIMEOUT);
	}
}
