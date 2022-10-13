/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 11, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <drivers/cs_Storage.h>
#include <logging/cs_Logger.h>
#include <cfg/cs_Strings.h>

// fds_record_update
// fds_record_write
// fds_record_find
// fds_record_delete
// fds_record_find_by_key
// fds_record_open
// fds_record_close
// fds_record_iterate
// fds_gc
// fds_flash_end_addr
// fds_init
// fds_register

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
 * Finds given type starting from given iterator. Sets lastFound iterator.
 */
template<class I>
cs_ret_code_t findFrom(CS_TYPE type, cs_state_id_t& id, I startIter) {
	auto it = findIf(_storage, matchType(type), startIter);
	if (it != _storage.end()) {
		id        = it->id;
		lastFound = it;
		return ERR_SUCCESS;
	}
	else {
		lastFound = NOT_FOUND();
		return ERR_NOT_FOUND;
	}
}

/**
 * Implements removal for the different matching options.
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
	LOGi("Storage::init()");
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
	eraseAllPages();
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::findFirst(CS_TYPE type, cs_state_id_t& id) {
	return findFrom(type, id, std::begin(_storage));
}

cs_ret_code_t Storage::findNext(CS_TYPE type, cs_state_id_t& id) {
	if(lastFound == NOT_FOUND()) {
		return ERR_NOT_FOUND;
	}
	// Resumes search from lastFound. Call findFirst to restart.

	auto searchFrom = lastFound;
	searchFrom++;
	return findFrom(type, id, searchFrom);
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

// ------------- mock doesn't do anything -------------

void Storage::setErrorCallback(cs_storage_error_callback_t callback) {
}

cs_ret_code_t Storage::garbageCollect() {
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::erasePages(const CS_TYPE doneEvent, void* startAddress, void* endAddress) {
	return ERR_SUCCESS;
}

// ------------ not implemented ------------

cs_ret_code_t Storage::read(cs_state_data_t& data) {
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::readV3ResetCounter(cs_state_data_t& data) {
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::readFirst(cs_state_data_t& data) {

	return ERR_SUCCESS;
}

cs_ret_code_t Storage::readNext(cs_state_data_t& data) {
	return ERR_SUCCESS;
}



void Storage::handleFileStorageEvent(fds_evt_t const* p_fds_evt) {}
void Storage::handleFlashOperationSuccess() {}
void Storage::handleFlashOperationError() {}

