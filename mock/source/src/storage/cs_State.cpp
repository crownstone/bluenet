/** Store settings in RAM or persistent memory.
 *
 * This files serves as a little indirection between loading data from RAM or FLASH.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 22, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <events/cs_Event.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>

// todo: implement
cs_state_data_t someState;

State::State() : _storage(NULL), _boardsConfig(NULL) {
}

State::~State() {

}

void State::init(boards_config_t* boardsConfig) {

}

cs_ret_code_t State::get(const CS_TYPE type, void *value, size16_t size) {
	return ERR_NOT_IMPLEMENTED;
}

bool State::isTrue(CS_TYPE type, const PersistenceMode mode) {
	return false;
}

cs_ret_code_t State::set(const CS_TYPE type, void *value, const size16_t size) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::set(const cs_state_data_t & data, PersistenceMode mode) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::remove(const CS_TYPE & type, cs_state_id_t id, const PersistenceMode mode) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::get(cs_state_data_t & data, const PersistenceMode mode) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::setInternal(const cs_state_data_t & data, const PersistenceMode mode) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::removeInternal(const CS_TYPE & type, cs_state_id_t id, const PersistenceMode mode) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::getDefaultValue(cs_state_data_t & data) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::findInRam(const CS_TYPE & type, cs_state_id_t id, size16_t & index_in_ram) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::storeInRam(const cs_state_data_t & data) {
	return ERR_NOT_IMPLEMENTED;
}


cs_ret_code_t State::storeInRam(const cs_state_data_t & data, size16_t & index_in_ram) {
	return ERR_NOT_IMPLEMENTED;
}

cs_state_data_t & State::addToRam(const CS_TYPE & type, cs_state_id_t id, size16_t size) {
	return someState;
}

cs_ret_code_t State::removeFromRam(const CS_TYPE & type, cs_state_id_t id) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::allocate(cs_state_data_t & data) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::loadFromRam(cs_state_data_t & data) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::storeInFlash(size16_t & index_in_ram) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::removeFromFlash(const CS_TYPE & type, const cs_state_id_t id) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::compareWithRam(const cs_state_data_t & data, uint32_t & cmp_result) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::verifySizeForGet(const cs_state_data_t & data) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::verifySizeForSet(const cs_state_data_t & data) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::getIds(CS_TYPE type, std::vector<cs_state_id_t>* & ids) {
	return ERR_NOT_IMPLEMENTED;
}
cs_ret_code_t State::getIdsFromFlash(const CS_TYPE & type, std::vector<cs_state_id_t>* & retIds) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::addId(const CS_TYPE & type, cs_state_id_t id) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::remId(const CS_TYPE & type, cs_state_id_t id) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::setThrottled(const cs_state_data_t & data, uint32_t periodSeconds) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t State::setDelayed(const cs_state_data_t & data, uint8_t delaySeconds) {
	return ERR_NOT_IMPLEMENTED;
}
cs_ret_code_t State::addToQueue(StateQueueOp operation, const CS_TYPE & type, cs_state_id_t id, uint32_t delayMs,
		const StateQueueMode mode) {
	return ERR_NOT_IMPLEMENTED;
}

void State::delayedStoreTick() {

}

void State::startWritesToFlash() {

}

void State::factoryReset() {
}

cs_ret_code_t State::cleanUp() {
	return ERR_NOT_IMPLEMENTED;
}

bool State::handleFactoryResetResult(cs_ret_code_t retCode) {
	return false;
}

void State::handleStorageError(cs_storage_operation_t operation, CS_TYPE type, cs_state_id_t id) {

}

void State::handleEvent(event_t & event) {

}
