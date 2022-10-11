/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 11, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Storage.h>
#include <common/cs_Types.h>

#include <iostream>

cs_ret_code_t Storage::init() {
	std::cout << "Storage::init()" << std::endl;
}

inline bool isInitialized();

void setErrorCallback(cs_storage_error_callback_t callback);

cs_ret_code_t findFirst(CS_TYPE type, cs_state_id_t& id);
cs_ret_code_t findNext(CS_TYPE type, cs_state_id_t& id);

cs_ret_code_t read(cs_state_data_t& data);
cs_ret_code_t readV3ResetCounter(cs_state_data_t& data);
cs_ret_code_t readFirst(cs_state_data_t& data);
cs_ret_code_t readNext(cs_state_data_t& data);

cs_ret_code_t write(const cs_state_data_t& data);

cs_ret_code_t remove(CS_TYPE type, cs_state_id_t id);
cs_ret_code_t remove(CS_TYPE type);
cs_ret_code_t remove(cs_state_id_t id);

cs_ret_code_t factoryReset();

cs_ret_code_t garbageCollect();

cs_ret_code_t eraseAllPages();
cs_ret_code_t erasePages(const CS_TYPE doneEvent, void* startAddress, void* endAddress);

uint8_t* allocate(size16_t& size);


void handleFileStorageEvent(fds_evt_t const* p_fds_evt);
void handleFlashOperationSuccess();
void handleFlashOperationError();
