/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 11, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Storage.h>

#include <iostream>

int main() {
	bool testFailed = false;
	cs_state_id_t out;
	cs_state_data_t dat;

	auto& s = Storage::getInstance();

	s.write(dat); // ignored

	s.init();

	if(s.findFirst(dat.type, out) != ERR_NOT_FOUND) {
		LOGw("writing before init should have been skipped");
		testFailed |= true;
	}

	// write a series of state data objects to the storage
	std::vector<cs_state_id_t> ids = {0,1,1,2,3,4,7,9};
	for (auto id : ids) {
		dat.id = id;
		s.write(dat);
	}

	std::vector<cs_state_id_t> idsUnique;
	for (auto id : ids) {
		if (std::find(std::begin(idsUnique), std::end(idsUnique), id) == std::end(idsUnique)) {
			idsUnique.push_back(id);
		}
	}

	// storage should contain the unique entries in that order.
	int expectedValueIndex = 0;
	for (cs_ret_code_t retcode = s.findFirst(dat.type, out); retcode == ERR_SUCCESS;
		 retcode               = s.findNext(dat.type, out)) {
		if (out != idsUnique[expectedValueIndex]){
			LOGw("found entry: %u, expected %u", out, idsUnique[expectedValueIndex]);
			testFailed |= true;
		}

		expectedValueIndex++;
	}

	for (int i = 0; i < 5; i++) {
		if(s.findNext(dat.type, out) == ERR_SUCCESS) {
			LOGw("a findFirst, findNext, ... sequence should not automatically restart");
			testFailed |= true;
		}
	}

	if(s.findFirst(dat.type, out) != ERR_SUCCESS) {
		LOGw("findFirst should return ERR_SUCCESS, since an object of given type is written to storage.");
		testFailed |= true;
	}

	s.eraseAllPages();

	if(s.findFirst(dat.type, out) != ERR_NOT_FOUND) {
		LOGw("findFirst should return ERR_NOT_FOUND, since an storage was cleared.");
		testFailed |= true;
	}

	return testFailed ? 1 : 0;
}
