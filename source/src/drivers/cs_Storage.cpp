/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Handlers.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <float.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <util/cs_BleError.h>

#include <climits>

#define LOGStorageInit LOGi
#define LOGStorageInfo LOGi
#define LOGStorageWrite LOGd
#define LOGStorageDebug LOGd
#define LOGStorageVerbose LOGd

Storage::Storage() {}

void storage_fds_evt_handler(void * p_event_data, [[maybe_unused]] uint16_t event_size) {
	Storage::getInstance().handleFileStorageEvent(reinterpret_cast<const fds_evt_t*>(p_event_data));
}

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

	LOGStorageVerbose("fds_init");
	fds_ret_code = NRF_SUCCESS;

	fds_evt_t event;
	event.id = FDS_EVT_INIT;
	event.result = 0;
	uint32_t retVal = app_sched_event_put(&event, sizeof(event), storage_fds_evt_handler);
	APP_ERROR_CHECK(retVal);

	LOGStorageInit("Storage init success, wait for event.");
	return getErrorCode(fds_ret_code);
}

void Storage::setErrorCallback(cs_storage_error_callback_t callback) {
	_errorCallback = callback;
}

cs_ret_code_t Storage::findFirst(CS_TYPE type, cs_state_id_t& id) {
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

cs_ret_code_t Storage::findNext(CS_TYPE type, cs_state_id_t& id) {
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

cs_ret_code_t Storage::findNextInternal(uint16_t recordKey, uint16_t& fileId) {
	if (!isValidRecordKey(recordKey)) {
		return ERR_WRONG_PARAMETER;
	}

	return ERR_NOT_FOUND;
}

/**
 * Iterate over all records, so that in case of duplicates, the last written record will be used to read.
 */
cs_ret_code_t Storage::read(cs_state_data_t& stateData) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	uint16_t fileId    = getFileId(stateData.id);
	if (!isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;

	if (csRetCode == ERR_NOT_FOUND) {
		LOGStorageDebug("Record not found");
	}
	return csRetCode;
}

cs_ret_code_t Storage::readV3ResetCounter(cs_state_data_t& stateData) {
	// Code copied from read() with only fileId changed.
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	uint16_t fileId    = FILE_KEEP_FOREVER;
	// clang-format off
	if (stateData.type != CS_TYPE::STATE_RESET_COUNTER
			|| stateData.id != 0
			|| !isValidRecordKey(recordKey)
			|| !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	// clang-format on
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;

	if (csRetCode == ERR_NOT_FOUND) {
		LOGStorageDebug("Record not found");
	}
	return csRetCode;
}

cs_ret_code_t Storage::readFirst(cs_state_data_t& stateData) {
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

cs_ret_code_t Storage::readNext(cs_state_data_t& stateData) {
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
cs_ret_code_t Storage::readNextInternal(uint16_t recordKey, uint16_t& fileId, uint8_t* buf, uint16_t size) {
	if (!isValidRecordKey(recordKey)) {
		return ERR_WRONG_PARAMETER;
	}
	cs_ret_code_t csRetCode = ERR_NOT_FOUND;

	return csRetCode;
}

cs_ret_code_t Storage::readRecord(fds_record_desc_t recordDesc, uint8_t* buf, uint16_t size, uint16_t& fileId) {
	ret_code_t fdsRetCode = FDS_ERR_NOT_FOUND;
	return getErrorCode(fdsRetCode);
}

cs_ret_code_t Storage::write(const cs_state_data_t& stateData) {
	return getErrorCode(writeInternal(stateData));
}

/**
 * When the space is exhausted, write requests return the error FDS_ERR_NO_SPACE_IN_FLASH, and you must
 * run garbage collection and wait for completion before repeating the call to the write function.
 */
ret_code_t Storage::writeInternal(const cs_state_data_t& stateData) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(stateData.type);
	uint16_t fileId    = getFileId(stateData.id);
	if (!isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return FDS_ERR_BUSY;
	}

	setBusy(recordKey);

	fds_evt_t event;
	event.id = FDS_EVT_WRITE;
	event.result = 0;
	event.write.record_key = recordKey;
	event.write.file_id = fileId;
	uint32_t retVal = app_sched_event_put(&event, sizeof(event), storage_fds_evt_handler);
	APP_ERROR_CHECK(retVal);

	LOGStorageVerbose("Started writing");

	return NRF_SUCCESS;
}

cs_ret_code_t Storage::remove(CS_TYPE type, cs_state_id_t id) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint16_t recordKey = to_underlying_type(type);
	uint16_t fileId    = getFileId(id);
	if (!isValidRecordKey(recordKey) || !isValidFileId(fileId)) {
		return ERR_WRONG_PARAMETER;
	}
	if (isBusy(recordKey)) {
		return ERR_BUSY;
	}
	LOGStorageDebug("Remove key=%u file=%u", recordKey, fileId);

	return ERR_SUCCESS;
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

	return ERR_SUCCESS;
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

	_removingFile = true;

	fds_evt_t event;
	event.id = FDS_EVT_DEL_FILE;
	event.result = 0;
	event.write.file_id = fileId;
	uint32_t retVal = app_sched_event_put(&event, sizeof(event), storage_fds_evt_handler);
	APP_ERROR_CHECK(retVal);

	return ERR_SUCCESS;;
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

	cs_ret_code_t retCode = continueFactoryReset();
	if (retCode == ERR_SUCCESS) {
		_performingFactoryReset = true;
	}
	return retCode;
}

cs_ret_code_t Storage::continueFactoryReset() {
	LOGStorageVerbose("continueFactoryReset");
	return garbageCollect();
}

cs_ret_code_t Storage::garbageCollect() {
	return getErrorCode(garbageCollectInternal());
}

ret_code_t Storage::garbageCollectInternal() {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}
	uint8_t enabled = nrf_sdh_is_enabled();
	if (!enabled) {
		LOGe("Softdevice is not enabled yet!");
	}
	if (isBusy()) {
		return FDS_ERR_BUSY;
	}

	LOGStorageVerbose("fds_gc");
	fds_evt_t event;
	event.id = FDS_EVT_GC;
	event.result = 0;
	uint32_t retVal = app_sched_event_put(&event, sizeof(event), storage_fds_evt_handler);
	APP_ERROR_CHECK(retVal);


	LOGStorageDebug("Started garbage collection");
	_collectingGarbage = true;
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::eraseAllPages() {
	LOGw("eraseAllPages");
	if (_initialized || isErasingPages()) {
		return ERR_NOT_AVAILABLE;
	}
	// The function fds_flash_end_addr() is available in patched SDKs
	uint32_t endAddr        = fds_flash_end_addr();
	uint32_t flashSizeWords = FDS_VIRTUAL_PAGES * FDS_VIRTUAL_PAGE_SIZE;
	uint32_t flashSizeBytes = flashSizeWords * sizeof(uint32_t);
	uint32_t startAddr      = endAddr - flashSizeBytes;

	// Check if pages are already erased.
	uint32_t* startAddrPointer = (uint32_t*)startAddr;
	bool isErased                  = true;
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

	return erasePages(CS_TYPE::EVT_STORAGE_PAGES_ERASED, (void*)startAddr, (void*)endAddr);
}

cs_ret_code_t Storage::erasePages(const CS_TYPE doneEvent, void* startAddressPtr, void* endAddressPtr) {
	if (_initialized || isErasingPages()) {
		return ERR_NOT_AVAILABLE;
	}
	uint32_t startAddr      = (uint32_t)startAddressPtr;
	uint32_t endAddr        = (uint32_t)endAddressPtr;
	uint32_t const pageSize = NRF_FICR->CODEPAGESIZE;
	uint32_t startPage      = startAddr / pageSize;
	uint32_t endPage        = endAddr / pageSize;
	if (startPage > endPage) {
		APP_ERROR_CHECK(NRF_ERROR_INVALID_ADDR);
	}

	LOGw("Erase flash pages: [0x%p - 0x%p) or [%u - %u)", startAddr, endAddr, startPage, endPage);
	_erasePage      = startPage;
	_eraseEndPage   = endPage;
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
	size16_t flashSize   = getPaddedSize(size);
	size16_t paddingSize = flashSize - size;
	uint8_t* ptr         = (uint8_t*)malloc(sizeof(uint8_t) * flashSize);
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

/**
 * Check if a record exists.
 * Warns when it's found multiple times, but doesn't delete them. That would require to make a copy of the first found
 * record_desc.
 * Returns the last found record.
 */
ret_code_t Storage::exists(cs_file_id_t fileId, uint16_t recordKey, fds_record_desc_t& record_desc, bool& result) {
	initSearch();
	result = false;
	while (fds_record_find(fileId, recordKey, &record_desc, &_findToken) == NRF_SUCCESS) {
		if (result) {
			LOGe("Duplicate record key=%u file=%u addr=%p", recordKey, fileId, _findToken.p_addr);
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

/*
 * The C++ implemention for isBusy would use std::find, but this has 400 bytes overhead.
 */
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
}

bool Storage::isBusy() {
	if (_collectingGarbage || _removingFile || _performingFactoryReset || !_busyRecordKeys.empty()) {
		LOGw("Busy: gc=%u rm_file=%u factoryReset=%u records=%u",
			 _collectingGarbage,
			 _removingFile,
			 _performingFactoryReset,
			 _busyRecordKeys.size());
		return true;
	}
	return false;
}

/**
 * Returns cs_ret_code_t given a ret_code_t from FDS. Note how some different return codes are mapped to the same error
 * code.
 */
cs_ret_code_t Storage::getErrorCode(ret_code_t code) {
	switch (code) {
		case NRF_SUCCESS: {
			return ERR_SUCCESS;
		}
		case FDS_ERR_NOT_FOUND: {
			return ERR_NOT_FOUND;
		}
		case FDS_ERR_NOT_INITIALIZED: {
			return ERR_NOT_INITIALIZED;
		}
		case FDS_ERR_NO_SPACE_IN_FLASH: {
			return ERR_NO_SPACE;
		}
		case FDS_ERR_NO_SPACE_IN_QUEUES: {
			[[fallthrough]];
		}
		case FDS_ERR_BUSY: {
			return ERR_BUSY;
		}
		case FDS_ERR_UNALIGNED_ADDR: {
			[[fallthrough]];
		}
		case FDS_ERR_INVALID_ARG: {
			[[fallthrough]];
		}
		case FDS_ERR_NULL_ARG: {
			return ERR_WRONG_PARAMETER;
		}
		default: {
			return ERR_UNSPECIFIED;
		}
	}
}

void Storage::handleWriteEvent(fds_evt_t const* p_fds_evt) {
	clearBusy(p_fds_evt->write.record_key);
	TYPIFY(EVT_STORAGE_WRITE_DONE) eventData;
	eventData.type = CS_TYPE(p_fds_evt->write.record_key);
	eventData.id   = getStateId(p_fds_evt->write.file_id);
	switch (p_fds_evt->result) {
		case NRF_SUCCESS: {
			LOGStorageWrite(
					"Write done, key=%u file=%u type=%u id=%u",
					p_fds_evt->del.record_key,
					p_fds_evt->del.file_id,
					to_underlying_type(eventData.type),
					eventData.id);
			event_t event(CS_TYPE::EVT_STORAGE_WRITE_DONE, &eventData, sizeof(eventData));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		default:
			LOGw("Write FDSerror=%u key=%u file=%u",
				 p_fds_evt->result,
				 p_fds_evt->write.record_key,
				 p_fds_evt->write.file_id);
			if (_errorCallback) {
				_errorCallback(CS_STORAGE_OP_WRITE, eventData.type, eventData.id);
			}
			else {
				LOGw("Unhandled");
			}
			break;
	}
}

void Storage::handleRemoveRecordEvent(fds_evt_t const* p_fds_evt) {
	clearBusy(p_fds_evt->del.record_key);
	TYPIFY(EVT_STORAGE_REMOVE_DONE) eventData;
	eventData.type = toCsType(p_fds_evt->del.record_key);
	eventData.id   = getStateId(p_fds_evt->del.file_id);
	switch (p_fds_evt->result) {
		case NRF_SUCCESS: {
			LOGStorageInfo(
					"Remove done, key=%u file=%u type=%u id=%u",
					p_fds_evt->del.record_key,
					p_fds_evt->del.file_id,
					to_underlying_type(eventData.type),
					eventData.id);
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
			LOGw("Remove FDSerror=%u key=%u file=%u",
				 p_fds_evt->result,
				 p_fds_evt->del.record_key,
				 p_fds_evt->del.file_id);
			if (_errorCallback) {
				_errorCallback(CS_STORAGE_OP_REMOVE, eventData.type, eventData.id);
			}
			else {
				LOGw("Unhandled");
			}
	}
}

void Storage::handleRemoveFileEvent(fds_evt_t const* p_fds_evt) {
	_removingFile    = false;
	cs_state_id_t id = getStateId(p_fds_evt->write.file_id);
	switch (p_fds_evt->result) {
		case NRF_SUCCESS: {
			LOGStorageInfo("Remove file done, file=%u id=%u", p_fds_evt->del.file_id, id);
			event_t event(CS_TYPE::EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE, &id, sizeof(id));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		default: {
			LOGw("Remove FDSerror=%u file=%u", p_fds_evt->result, p_fds_evt->del.file_id);
			if (_errorCallback) {
				_errorCallback(CS_STORAGE_OP_REMOVE_ALL_VALUES_WITH_ID, CS_TYPE::CONFIG_DO_NOT_USE, id);
			}
			else {
				LOGw("Unhandled");
			}
		}
	}
}

void Storage::handleGarbageCollectionEvent(fds_evt_t const* p_fds_evt) {
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
		case FDS_ERR_OPERATION_TIMEOUT: {
			LOGw("Garbage collection timeout");
			[[fallthrough]];
		}
		default: {
			if (_errorCallback) {
				_errorCallback(CS_STORAGE_OP_GC, CS_TYPE::CONFIG_DO_NOT_USE, 0);
			}
			else {
				LOGw("Unhandled GC error: %u", p_fds_evt->result);
			}
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
void Storage::handleFileStorageEvent(fds_evt_t const* p_fds_evt) {
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
				APP_ERROR_HANDLER(p_fds_evt->result);
			}
			break;
		}
		case FDS_EVT_WRITE: {
			[[fallthrough]];
		}
		case FDS_EVT_UPDATE: {
			handleWriteEvent(p_fds_evt);
			break;
		}
		case FDS_EVT_DEL_RECORD: {
			handleRemoveRecordEvent(p_fds_evt);
			break;
		}
		case FDS_EVT_DEL_FILE: {
			handleRemoveFileEvent(p_fds_evt);
			break;
		}
		case FDS_EVT_GC: {
			handleGarbageCollectionEvent(p_fds_evt);
			break;
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
		APP_ERROR_HANDLER(NRF_ERROR_TIMEOUT);
	}
}
