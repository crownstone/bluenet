/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 11, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Storage.h>

#include <iostream>

int main() {
	auto& s = Storage::getInstance();
	cs_state_data_t dat;
	s.write(dat);
	s.init();
	s.write(dat);
	s.write(dat);
	s.write(dat);
	std::cout << "hello from storage write test. " << (s.isInitialized()?"yay":"nay") << std::endl;
	return 0;
}
