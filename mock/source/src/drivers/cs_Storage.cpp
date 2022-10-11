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
#define NOT_FOUND std::end(_storage)


void logStateDataEntry(const cs_state_data_t& dat, std::string prefix = "") {
	LOGd("%s{id: %u, type:%u, size:%u, value:%u}", prefix.c_str(), dat.id, dat.type, dat.size, dat.value);
}

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

// --- std algorithms

/**
 * Factor out all the cruft from std::find_if and use begin/end as sane defaults.
 */
template<class C, class Pred>
int findIf(C& container, const Pred& pred) {
	return std::find_if(std::begin(container), std::end(container), pred);
}

/**
 * Deleting elements in vectors is not pretty until C++20. See std::erase_if
 */
template<class C, class Pred>
int eraseIf(C& container, const Pred& pred) {
	auto oldEndIt = std::end(container);
	auto newEndIt = std::remove_if(std::begin(container), oldEndIt, pred);
	int removedCount = std::distance(newEndIt, oldEndIt);
	container.erase(newEndIt, oldEndIt);
	return removedCount;
}
//
///**
// * returns iterator to object in _storage with id and type equal to target. else std::end(_storage).
// */
//auto find(cs_state_data_t target) {
//	return find_if(_storage, equalTo(target));
//}
//
///**
// * returns iterator to object in _storage with id and type equal to target. else std::end(_storage).
// */
//auto find(cs_state_id_t targetId, CS_TYPE targetType) {
//	return find_if(_storage, matchIdType(targetId, targetType));
//}

// ------------- implemented -------------

cs_ret_code_t Storage::init() {
	LOGd("Storage::init()");
	_initialized = true;
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::write(const cs_state_data_t& data) {
	if (!_initialized) {
		LOGe(STR_ERR_NOT_INITIALIZED);
		return ERR_NOT_INITIALIZED;
	}

	int removeCount = eraseIf(_storage, equalTo(data));
	if(removeCount) {
		LOGd("removed old entries: %u", removeCount);
	}

	_storage.push_back(data);

	for(auto d : _storage) {
		logStateDataEntry(d, " * ");
	}

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
void Storage::handleFileStorageEvent(fds_evt_t const* p_fds_evt) {}
void Storage::handleFlashOperationSuccess() {}
void Storage::handleFlashOperationError() {}

cs_ret_code_t Storage::findFirst(CS_TYPE type, cs_state_id_t& id) {
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::findNext(CS_TYPE type, cs_state_id_t& id) {
	return ERR_SUCCESS;
}

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


cs_ret_code_t Storage::remove(CS_TYPE type, cs_state_id_t id) {
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::remove(CS_TYPE type) {
	return ERR_SUCCESS;
}

cs_ret_code_t Storage::remove(cs_state_id_t id) {
	return ERR_SUCCESS;
}

