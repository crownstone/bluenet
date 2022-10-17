/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 11, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <drivers/cs_Storage.h>
#include <events/cs_Event.h>
#include <logging/cs_Logger.h>
#include <cfg/cs_Strings.h>


#define LOGStorageMockDebug LOGd

// this contains the data of the static storage instance.
std::vector<cs_state_data_t> _storage;


// --- utils

// --- comparison operations

auto matchId(cs_state_id_t targetId) {
	return [=](cs_state_data_t rec) { return rec.id == targetId;};
}

auto matchType(CS_TYPE targetType) {
	return [=](cs_state_data_t rec) { return rec.type == targetType;};
}

auto matchIdType(cs_state_id_t targetId, CS_TYPE targetType) {
	return [=](cs_state_data_t rec) {
		return rec.id == targetId && rec.type == targetType;
	};
}

auto equalTo(cs_state_data_t target) {
	return matchIdType(target.id, target.type);
}

/**
 *  End is used as indicator value for 'not found'.
 *  Use this function rather than std::end directly.
 */
auto NOT_FOUND() {
	return std::end(_storage);
}

/**
 *  To emulate findNext, we keep track of the last find result.
 *  It is reset by findFirst and read by findNext.
 */
auto lastFound = NOT_FOUND();

/**
 * Implements removal for the different matching predicates.
 */
template<class P>
cs_ret_code_t _remove(Storage& s, P predicate){
	if (!s.isInitialized()) {
		return ERR_NOT_INITIALIZED;
	}
	if (s.isBusy()){
		return ERR_BUSY;
	}

	int eraseCount = eraseIf(_storage, predicate);

	if(eraseCount == 0) {
		return ERR_NOT_FOUND;
	}

	return ERR_SUCCESS;
}

/**
 * Implements reading of data and corresponding return values
 * foundIter: iterator where target should have been found.
 */
template<class I>
cs_ret_code_t _read(Storage& s, cs_state_data_t& data, I foundIter) {
	if(s.isBusy()) {
		return ERR_BUSY;
	}

	if (foundIter == NOT_FOUND()) {
		return ERR_NOT_FOUND;
	}

	if(foundIter->size != data.size) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	data.value = foundIter->value;
	return ERR_SUCCESS;
}

/**
 * Finds given type starting from given iterator using `equalTo`.
 * If found, fills in `data.id`.
 *
 * Sets lastFound iterator.
 */
template<class I>
cs_ret_code_t _readFrom(Storage& s, cs_state_data_t& data, I startIter) {
	lastFound = findIf(_storage, equalTo(data), startIter);

	if (lastFound == NOT_FOUND()) {
		return ERR_NOT_FOUND;
	}

	return _read(s, data, lastFound);
}

/**
 * Finds given type starting from given iterator using `matchType`.
 * If found, fills in `data.id`.
 *
 * Sets lastFound iterator.
 */
template <class I>
cs_ret_code_t _findFrom(CS_TYPE type, cs_state_id_t& id, I startIter) {
	lastFound = findIf(_storage, matchType(type), startIter);

	if (lastFound == NOT_FOUND()) {
		return ERR_NOT_FOUND;
	}

	id = lastFound->id;
	return ERR_SUCCESS;
}

/**
 * print cs_state_Data_t object (with prefix).
 */
void logStateDataEntry(const cs_state_data_t& dat, std::string prefix = "") {
	LOGd("%s{id: %u, type:%u, size:%u, value:%u}", prefix.c_str(), dat.id, dat.type, dat.size, dat.value);
}


// --- std algorithm wrappers

/**
 * Factor out all the cruft from std::find_if and use begin/end as sane defaults.
 *
 * @return: first element matching
 */
template<class C, class Pred>
auto findIf(C& container, const Pred& pred) {
	return std::find_if(std::begin(container), std::end(container), pred);
}

template<class C, class Pred, class I>
auto findIf(C& container, const Pred& pred, I startIter) {
	return std::find_if(startIter, std::end(container), pred);
}

/**
 * Deleting elements in vectors is not pretty until C++20. See std::erase_if.
 * This wrapper makes it somewhat easier.
 *
 * @return: number of erased elements.
 */
template<class C, class Pred>
int eraseIf(C& container, const Pred& pred) {
	auto oldEndIt = std::end(container);
	auto newEndIt = std::remove_if(std::begin(container), oldEndIt, pred);
	int removedCount = std::distance(newEndIt, oldEndIt);
	container.erase(newEndIt, oldEndIt);
	return removedCount;
}

// ------------- implemented -------------

cs_ret_code_t Storage::init() {
	LOGi("Mock Storage::init()");
	_initialized = true;
	return ERR_SUCCESS;
}

bool Storage::isBusy() {
	return false;
}

cs_ret_code_t Storage::write(const cs_state_data_t& data) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}

	int removeCount = eraseIf(_storage, equalTo(data));
	if(removeCount) {
		LOGd("removed %u old entrie(s)", removeCount);
	}

	LOGStorageMockDebug("Storage::write pushing back data.");
	_storage.push_back(data);

	return ERR_SUCCESS;
}

cs_ret_code_t Storage::eraseAllPages() {
	_storage.clear();
	return ERR_SUCCESS;
}

uint8_t* Storage::allocate(size16_t& size) {
	return new uint8_t[size];
}

cs_ret_code_t Storage::factoryReset() {
	if (!_initialized) {
		return ERR_NOT_INITIALIZED;
	}
	if (isBusy()) {
		return ERR_BUSY;
	}
	_performingFactoryReset = true;
	eraseAllPages();
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::findFirst(CS_TYPE type, cs_state_id_t& id) {
	return _findFrom(type, id, std::begin(_storage));
}

cs_ret_code_t Storage::findNext(CS_TYPE type, cs_state_id_t& id) {
	if(lastFound == NOT_FOUND()) {
		return ERR_NOT_FOUND;
	}
	// Resumes search from lastFound. Call findFirst to restart.

	auto searchFrom = lastFound;
	searchFrom++;
	return _findFrom(type, id, searchFrom);
}

cs_ret_code_t Storage::remove(CS_TYPE type, cs_state_id_t id) {
	return _remove(*this, matchIdType(id, type));
}

cs_ret_code_t Storage::remove(CS_TYPE type) {
	return _remove(*this, matchType(type));
}

cs_ret_code_t Storage::remove(cs_state_id_t id) {
	return _remove(*this, matchId(id));
}

cs_ret_code_t Storage::read(cs_state_data_t& data) {
	// start reading from the beginning
	cs_ret_code_t prevRetCode = readFirst(data);

	// continue, as long as it is successful
	for(cs_ret_code_t currentRetCode = prevRetCode;
			currentRetCode == ERR_SUCCESS;
			currentRetCode = readNext(data)){
		prevRetCode=currentRetCode;
	}

	// returns result of readFirst if it isn't ERR_SUCCESS, else ERR_SUCCESS,
	// since there was a successful read at some point.
	return prevRetCode;
}

cs_ret_code_t Storage::readFirst(cs_state_data_t& data) {
	return _readFrom(*this, data, std::begin(_storage));
}

cs_ret_code_t Storage::readNext(cs_state_data_t& data) {
	if(lastFound == NOT_FOUND()) {
		return ERR_NOT_FOUND;
	}
	// Resumes search from lastFound. Call findFirst to restart.

	auto searchFrom = lastFound;
	searchFrom++;
	return _readFrom(*this, data, std::begin(_storage));
}

cs_ret_code_t Storage::readV3ResetCounter(cs_state_data_t& data) {
	return ERR_NOT_FOUND;
}

void Storage::handleFileStorageEvent(fds_evt_t const* p_fds_evt) {
	if(p_fds_evt == nullptr) {
		return;
	}

	const fds_evt_t& fds_evt = *p_fds_evt;

	switch (fds_evt.id) {
		case FDS_EVT_INIT: {
			event_t event(CS_TYPE::EVT_STORAGE_INITIALIZED);
			event.dispatch();

			break;
		}
		case FDS_EVT_WRITE:
		case FDS_EVT_UPDATE: {
			TYPIFY(EVT_STORAGE_WRITE_DONE) eventData;
			eventData.type = toCsType(p_fds_evt->write.record_key);
			eventData.id   = getStateId(p_fds_evt->write.file_id);

			event_t event(CS_TYPE::EVT_STORAGE_WRITE_DONE, &eventData, sizeof(eventData));
			event.dispatch();

			break;
		}
		case FDS_EVT_DEL_RECORD: {

			TYPIFY(EVT_STORAGE_REMOVE_DONE) eventData;
			eventData.type = toCsType(p_fds_evt->del.record_key);
			eventData.id   = getStateId(p_fds_evt->del.file_id);

			event_t event(CS_TYPE::EVT_STORAGE_REMOVE_DONE, &eventData, sizeof(eventData));
			event.dispatch();
			break;
		}
		case FDS_EVT_DEL_FILE: {
			cs_state_id_t id = getStateId(p_fds_evt->write.file_id);

			event_t event(CS_TYPE::EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE, &id, sizeof(id));
			event.dispatch();
			break;
		}
		case FDS_EVT_GC: {
			if (_performingFactoryReset) {
				_performingFactoryReset = false;
				event_t resetEvent(CS_TYPE::EVT_STORAGE_FACTORY_RESET_DONE);
				resetEvent.dispatch();
				return;
			}
			else {
				event_t event(CS_TYPE::EVT_STORAGE_GC_DONE);
				event.dispatch();
				return;
			}
			break;
		}
	}
}

uint16_t Storage::getFileId(cs_state_id_t valueId) {
	// Old configs were all at file id FILE_CONFIGURATION.
	// So for id 0, we want to get FILE_CONFIGURATION.
	return valueId + FILE_CONFIGURATION;
}

cs_state_id_t Storage::getStateId(uint16_t fileId) {
	return fileId - FILE_CONFIGURATION;
}


void Storage::setErrorCallback(cs_storage_error_callback_t callback) {
	// TODO: not implemented
}

cs_ret_code_t Storage::garbageCollect() {
	// TODO: not implemented
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::erasePages(const CS_TYPE doneEvent, void* startAddress, void* endAddress) {
	// TODO: not implemented
	return ERR_SUCCESS;
}

// ------------ not implemented ------------

void Storage::handleFlashOperationSuccess() {}
void Storage::handleFlashOperationError() {}

