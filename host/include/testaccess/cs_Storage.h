/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 14, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <test/cs_TestAccess.h>
#include <drivers/cs_Storage.h>
#include <components/libraries/fds/fds.h>

template <>
class TestAccess<Storage> {
	Storage& _storage;

public:
	TestAccess(Storage& storage) : _storage(storage) {}

	void emulateStorageEvent(fds_evt_t fds_evt) {
		_storage.handleFileStorageEvent(&fds_evt);
	}
};
