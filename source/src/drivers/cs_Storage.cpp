/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <climits>
#include <common/cs_Handlers.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <float.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <util/cs_BleError.h>
//#include <algorithm>

#define NR_CONFIG_ELEMENTS SIZEOF_ARRAY(config)

// Define as LOGd to get debug logs.
#define LOGStorageDebug LOGnone

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

	if (_initialized) {
		LOGi("Already initialized");
		return ERR_SUCCESS;
	}

	if (!_registeredFds) {
		LOGStorageDebug("fds_register");
		fds_ret_code = fds_register(fds_evt_handler);
		if (fds_ret_code != FDS_SUCCESS) {
			LOGe("Registering FDS event handler failed (err=%i)", fds_ret_code);
			return getErrorCode(fds_ret_code);
		}
		_registeredFds = true;
	}

	LOGStorageDebug("fds_init");
	fds_ret_code = fds_init();
	if (fds_ret_code != FDS_SUCCESS) {
		LOGe("Init FDS failed (err=%i)", fds_ret_code);
		return getErrorCode(fds_ret_code);
	}

	LOGi("Storage init success, wait for event.");
	return getErrorCode(fds_ret_code);
}

void Storage::setErrorCallback(cs_storage_error_callback_t callback) {
	_errorCallback = callback;
}

cs_ret_code_t Storage::findFirst(CS_TYPE type, uint16_t & id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
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

cs_ret_code_t Storage::findNext(CS_TYPE type, uint16_t & id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
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
	fds_record_t record;
	fds_record_desc_t recordDesc;
	fds_flash_record_t flashRecord;
	ret_code_t fdsRetCode;
	while (fds_record_find_by_key(recordKey, &recordDesc, &_findToken) == FDS_SUCCESS) {
		fdsRetCode = fds_record_open(&recordDesc, &flashRecord);
		if (fdsRetCode == FDS_SUCCESS) {
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
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	uint16_t fileId = getFileId(stateData.id);
	fds_record_desc_t recordDesc;
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;
	bool done = false;
	LOGd("Read record %u", recordKey);
	initSearch();
	while (fds_record_find(fileId, recordKey, &recordDesc, &_findToken) == FDS_SUCCESS) {
		if (done) {
			LOGe("Duplicate record key=%u addr=%p", recordKey, _findToken.p_addr);
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
		LOGd("Record not found");
	}
	return csRetCode;
}

cs_ret_code_t Storage::readFirst(cs_state_data_t & stateData) {
	if (!_initialized) {
		LOGe("Storage not initialized");
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
		LOGe("Storage not initialized");
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
	fds_record_t record;
	fds_record_desc_t recordDesc;
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;
	while (fds_record_find_by_key(recordKey, &recordDesc, &_findToken) == FDS_SUCCESS) {
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
	case FDS_SUCCESS: {
		cs_ret_code_t csRetCode = ERR_SUCCESS;
		LOGStorageDebug("Opened record id=%u addr=%p", flashRecord.p_header->record_id, recordDesc.p_record);
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
		if (fds_record_close(&recordDesc) != FDS_SUCCESS) {
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
	if (isBusy()) {
		return FDS_ERR_BUSY;
	}
	LOGStorageDebug("fds_gc");
	fds_ret_code = fds_gc();
	if (fds_ret_code != FDS_SUCCESS) {
		LOGw("Failed to start garbage collection (err=%i)", fds_ret_code);
	}
	else {
		LOGd("Started garbage collection");
		_collectingGarbage = true;
	}
	return fds_ret_code;
}

cs_ret_code_t Storage::write(cs_file_id_t file_id, const cs_state_data_t & file_data) {
	return getErrorCode(writeInternal(file_id, file_data));
}

/**
 * When the space is exhausted, write requests return the error FDS_ERR_NO_SPACE_IN_FLASH, and you must
 * run garbage collection and wait for completion before repeating the call to the write function.
 */
ret_code_t Storage::writeInternal(cs_file_id_t file_id, const cs_state_data_t & file_data) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(file_data.type);
	if (isBusy(recordKey)) {
		return FDS_ERR_BUSY;
	}
	fds_record_t record;
	fds_record_desc_t record_desc;
	ret_code_t fds_ret_code;

	record.file_id           = file_id;
	record.key               = recordKey;
	record.data.p_data       = file_data.value;
	// Assume the allocation was done by storage.
	// Size is in bytes, each word is 4B.
	record.data.length_words = getPaddedSize(file_data.size) >> 2;
	LOGd("Write record=%u or type=%u", record.key, recordKey);
	LOGStorageDebug("Data=%p word size=%u", record.data.p_data, record.data.length_words);

	bool f_exists = false;
	fds_ret_code = exists(file_id, recordKey, record_desc, f_exists);
	if (f_exists) {
		LOGStorageDebug("Update file %u record %u", file_id, record.key);
		fds_ret_code = fds_record_update(&record_desc, &record);
	}
	else {
		LOGStorageDebug("Write file %u, record %u, ptr %p", file_id, record.key, record.data.p_data);
		fds_ret_code = fds_record_write(&record_desc, &record);
	}
	switch(fds_ret_code) {
	case FDS_SUCCESS:
		setBusy(recordKey);
		LOGStorageDebug("Started writing");
		break;
	case FDS_ERR_NO_SPACE_IN_FLASH: {
		LOGi("Flash is full, start garbage collection");
		ret_code_t gc_ret_code = garbageCollect();
		if (gc_ret_code == FDS_SUCCESS) {
			fds_ret_code = FDS_ERR_BUSY;
		}
		else {
			LOGe("Failed to start GC: %u", gc_ret_code);
			fds_ret_code = gc_ret_code;
		}
		break;
	}
	case FDS_ERR_NO_SPACE_IN_QUEUES:
	case FDS_ERR_BUSY:
		break;
	default:
		LOGw("Unhandled write error: %u", fds_ret_code);
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

/*
 * Default id = 0
 * Old configs were all at file id FILE_CONFIGURATION.
 * So for id 0, we want to get FILE_CONFIGURATION.
 */
uint16_t Storage::getFileId(uint16_t valueId) {
	return valueId + FILE_CONFIGURATION;
}
uint16_t Storage::getStateId(uint16_t fileId) {
	return fileId - FILE_CONFIGURATION;
}

void Storage::print(const std::string & prefix, CS_TYPE type) {
	LOGd("%s %s (%i)", prefix.c_str(), TypeName(type), type);
}






//cs_ret_code_t Storage::remove(cs_file_id_t file_id) {
//	if (!_initialized) {
//		LOGe("Storage not initialized");
//		return ERR_NOT_INITIALIZED;
//	}
//	if (isBusy()) {
//		return ERR_BUSY;
//	}
//	LOGi("Delete file, invalidates all configuration values, but does not remove them yet!");
//	ret_code_t fds_ret_code;
//	fds_ret_code = fds_file_delete(file_id);
//	if (fds_ret_code == FDS_SUCCESS) {
//		_removingFile = true;
//	}
//	return getErrorCode(fds_ret_code);
//}

cs_ret_code_t Storage::remove(CS_TYPE type, uint16_t id) {
	if (!_initialized) {
		LOGe("Storage not initialized");
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(type);
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	uint16_t file_id = getFileId(id);
	fds_record_desc_t record_desc;
	ret_code_t fds_ret_code;

	fds_ret_code = FDS_ERR_NOT_FOUND;

	// Go through all records with given file_id and key (can be multiple).
	// Record key can be set busy multiple times.
	initSearch();
	while (fds_record_find(file_id, recordKey, &record_desc, &_findToken) == FDS_SUCCESS) {
		fds_ret_code = fds_record_delete(&record_desc);
		if (fds_ret_code == FDS_SUCCESS) {
			setBusy(recordKey);
		}
		else {
			break;
		}
	}
	return getErrorCode(fds_ret_code);
}

ret_code_t Storage::exists(cs_file_id_t file_id, uint16_t recordKey, bool & result) {
	fds_record_desc_t record_desc;
	return exists(file_id, recordKey, record_desc, result);
}

/**
 * Check if a record exists.
 * Warns when it's found multiple times, but doesn't delete them. That would require to make a copy of the first found
 * record_desc.
 * Returns the last found record.
 */
ret_code_t Storage::exists(cs_file_id_t file_id, uint16_t recordKey, fds_record_desc_t & record_desc, bool & result) {
	initSearch();
	result = false;
	while (fds_record_find(file_id, recordKey, &record_desc, &_findToken) == FDS_SUCCESS) {
		if (result) {
			LOGe("Duplicate record key=%u addr=%p", recordKey, _findToken.p_addr);
//			fds_record_delete(&record_desc);
		}
		if (!result) {
			result = true;
		}
	}
	return ERR_SUCCESS;
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

void Storage::setBusy(uint16_t recordKey) {
	_busy_record_keys.push_back(recordKey);
}

void Storage::clearBusy(uint16_t recordKey) {
	for (auto it = _busy_record_keys.begin(); it != _busy_record_keys.end(); it++) {
		if (*it == recordKey) {
			_busy_record_keys.erase(it);
			return;
		}
	}
}

bool Storage::isBusy(uint16_t recordKey) {
	if (_collectingGarbage || _removingFile) {
		LOGw("Busy: gc=%u rm_file=%u", _collectingGarbage, _removingFile);
		return true;
	}
	for (auto it = _busy_record_keys.begin(); it != _busy_record_keys.end(); it++) {
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
	if (_collectingGarbage || _removingFile || !_busy_record_keys.empty()) {
		LOGw("Busy: gc=%u rm_file=%u records=%u", _collectingGarbage, _removingFile, _busy_record_keys.size());
		return true;
	}
	return false;
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
	switch (p_fds_evt->result) {
	case FDS_SUCCESS: {
		CS_TYPE type = CS_TYPE(p_fds_evt->write.record_key);
		LOGd("Write done, record=%u or type=%u", p_fds_evt->write.record_key, to_underlying_type(type));
		event_t event(CS_TYPE::EVT_STORAGE_WRITE_DONE, &type, sizeof(type));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	default:
		LOGw("Write FDSerror=%u record=%u", p_fds_evt->result, p_fds_evt->write.record_key);
		if (_errorCallback) {
			CS_TYPE type = toCsType(p_fds_evt->write.record_key);
			_errorCallback(CS_STORAGE_OP_WRITE, p_fds_evt->write.file_id, type);
		}
		else {
			LOGw("Unhandled write error: %u record=%u", p_fds_evt->result, p_fds_evt->write.record_key);
		}
		break;
	}
}

void Storage::handleRemoveRecordEvent(fds_evt_t const * p_fds_evt) {
	clearBusy(p_fds_evt->del.record_key);
	switch (p_fds_evt->result) {
	case FDS_SUCCESS: {
		CS_TYPE type = CS_TYPE(p_fds_evt->del.record_key);
		LOGi("Remove done, record=%u or type=%u", p_fds_evt->del.record_key, to_underlying_type(type));
		event_t event(CS_TYPE::EVT_STORAGE_REMOVE_DONE, &type, sizeof(type));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	default:
		LOGw("Remove FDSerror=%u record=%u", p_fds_evt->result, p_fds_evt->del.record_key);
		if (_errorCallback) {
			CS_TYPE type = toCsType(p_fds_evt->del.record_key);
			_errorCallback(CS_STORAGE_OP_REMOVE, p_fds_evt->del.file_id, type);
		}
		else {
			LOGw("Unhandled rem record error: %u record=%u", p_fds_evt->result, p_fds_evt->del.record_key);
		}
	}
}

void Storage::handleRemoveFileEvent(fds_evt_t const * p_fds_evt) {
	_removingFile = false;
	switch (p_fds_evt->result) {
	case FDS_SUCCESS: {
		cs_file_id_t fileId = p_fds_evt->del.file_id;
		LOGi("Remove file done, file_id=%u", fileId);
		event_t event(CS_TYPE::EVT_STORAGE_REMOVE_FILE_DONE, &fileId, sizeof(fileId));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	default:
		LOGw("Remove FDSerror=%u file=%u", p_fds_evt->result, p_fds_evt->del.file_id);
		if (_errorCallback) {
			_errorCallback(CS_STORAGE_OP_REMOVE_FILE, p_fds_evt->del.file_id, CS_TYPE::CONFIG_DO_NOT_USE);
		}
		else {
			LOGw("Unhandled rem file error: %u file_id=%u", p_fds_evt->result, p_fds_evt->del.file_id);
		}
	}
}

void Storage::handleGarbageCollectionEvent(fds_evt_t const * p_fds_evt) {
	_collectingGarbage = false;
	switch (p_fds_evt->result) {
	case FDS_SUCCESS: {
		LOGi("Garbage collection successful");
		event_t event(CS_TYPE::EVT_STORAGE_GC_DONE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case FDS_ERR_OPERATION_TIMEOUT:
		LOGw("Garbage collection timeout");
	default:
		if (_errorCallback) {
			_errorCallback(CS_STORAGE_OP_GC, 0, CS_TYPE::CONFIG_DO_NOT_USE);
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
	LOGStorageDebug("FS: res=%u id=%u", p_fds_evt->result, p_fds_evt->id);
	switch(p_fds_evt->id) {
	case FDS_EVT_INIT: {
		if (p_fds_evt->result == FDS_SUCCESS) {
			LOGd("Storage initialized");
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

cs_ret_code_t Storage::eraseAllPages() {
	if (_initialized || isErasingPages()) {
		return ERR_NOT_AVAILABLE;
	}
	uint32_t const pageSize = NRF_FICR->CODEPAGESIZE;
	uint32_t endAddr = fds_flash_end_addr();
	uint32_t flashSizeWords = FDS_VIRTUAL_PAGES * FDS_VIRTUAL_PAGE_SIZE;
	uint32_t flashSizeBytes = flashSizeWords * sizeof(uint32_t);
	uint32_t startAddr = endAddr - flashSizeBytes;
	uint32_t startPage = startAddr / pageSize;
	uint32_t endPage = endAddr / pageSize;
	if (startPage > endPage) {
		APP_ERROR_CHECK(NRF_ERROR_INVALID_ADDR);
	}

	// Check if pages are already erased.
	uint32_t* startAddrPointer = (uint32_t*)startAddr;
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
	LOGw("Erase flash pages used for FDS: 0x%x - 0x%x", startAddr, endAddr);
	_erasePage = startPage;
	_eraseEndPage = endPage;
	eraseNextPage();
	return ERR_SUCCESS;
}

bool Storage::isErasingPages() {
	return (_erasePage != 0 && _eraseEndPage != 0 && _erasePage <= _eraseEndPage);
}

void Storage::eraseNextPage() {
	if (_erasePage != 0 && _eraseEndPage != 0 && _erasePage <= _eraseEndPage) {
		if (_eraseEndPage == _erasePage) {
			LOGi("Done erasing pages");
			event_t event(CS_TYPE::EVT_STORAGE_PAGES_ERASED);
			EventDispatcher::getInstance().dispatch(event);
		}
		else {
			uint32_t result = NRF_ERROR_BUSY;
			while (result == NRF_ERROR_BUSY) {
				LOGStorageDebug("Erase page %u", _erasePage);
				result = sd_flash_page_erase(_erasePage);
				LOGi("Erase result: %u", result);
			}
			APP_ERROR_CHECK(result);
			_erasePage++;
			LOGi("Wait for flash operation success");
		}
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
