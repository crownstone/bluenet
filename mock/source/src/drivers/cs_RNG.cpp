/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 15 Aug., 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma message("hello from rng mock")
#include <drivers/cs_RNG.h>

#include <random>
std:: minstd_rand StdRand;

RNG::RNG(){
	StdRand.seed(0x12345678);
}

void RNG::fillBuffer(uint8_t* buffer, uint8_t length) {
	for (int i = 0; i < length; i++) {
		// quick and dirty
		buffer[i] = StdRand() & 0xff;
	}
}

uint32_t RNG::getRandom32() {
	return StdRand();
};

uint16_t RNG::getRandom16() {
	return StdRand() & 0xffff;
}

uint8_t RNG::getRandom8() {
	return StdRand() & 0xff;
}
