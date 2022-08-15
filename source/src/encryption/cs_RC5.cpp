/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <encryption/cs_KeysAndAccess.h>
#include <encryption/cs_RC5.h>

// Rotate functions for 16 bit integers
#define ROTL_16BIT(x, shift) ((x) << (shift) | (x) >> (16 - (shift)))
#define ROTR_16BIT(x, shift) ((x) >> (shift) | (x) << (16 - (shift)))

// Magic numbers for RC5 with 16 bit words.
#define RC5_16BIT_P 0xB7E1
#define RC5_16BIT_Q 0x9E37

RC5::RC5() {}

void RC5::init() {
	initKey(EncryptionAccessLevel::LOCALIZATION);
}

bool RC5::initKey(EncryptionAccessLevel accessLevel) {
	uint8_t key[ENCRYPTION_KEY_LENGTH];
	bool result = KeysAndAccess::getInstance().getKey(accessLevel, key, sizeof(key));
	if (!result) {
		return false;
	}

	return prepareKey(key, sizeof(key));
}

// See https://en.wikipedia.org/wiki/RC5
bool RC5::prepareKey(uint8_t* key, uint8_t keyLength) {
	if (keyLength != RC5_KEYLEN) {
		return false;
	}
	int keyLenWords =
			((RC5_KEYLEN - 1) / sizeof(uint16_t)) + 1;  // c - The length of the key in words (or 1, if keyLength = 0).
	int loops               = 3 * (RC5_NUM_SUBKEYS > keyLenWords ? RC5_NUM_SUBKEYS : keyLenWords);
	uint16_t L[keyLenWords] = {
			0};  // L[] - A temporary working array used during key scheduling. initialized to the key in words.

	for (int i = 0; i < keyLenWords; ++i) {
		L[i] = (key[2 * i + 1] << 8) + key[2 * i];
		//		LOGd("L[i]=%u", L[i]);
	}

	_subKeys[0] = RC5_16BIT_P;
	for (int i = 1; i < RC5_NUM_SUBKEYS; ++i) {
		_subKeys[i] = _subKeys[i - 1] + RC5_16BIT_Q;
	}

	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t a = 0;
	uint16_t b = 0;
	uint16_t sum;
	for (int k = 0; k < loops; ++k) {
		//		LOGd("i=%u j=%u a=%u b=%u L[j]=%u key[i]=%u", i, j, a, b, L[j], _rc5SubKeys[i]);
		sum         = _subKeys[i] + a + b;
		a           = ROTL_16BIT(sum, 3);
		_subKeys[i] = a;

		sum         = L[j] + a + b;
		b           = ROTL_16BIT(sum, (a + b) % 16);
		L[j]        = b;
		//		LOGd("  i=%u j=%u a=%u b=%u L[j]=%u key[i]=%u", i, j, a, b, L[j], _rc5SubKeys[i]);
		++i;
		++j;
		i %= RC5_NUM_SUBKEYS;
		j %= keyLenWords;
	}
	//	_log(SERIAL_DEBUG, false, "key=");
	//	CsUtils::printArray(key, keyLength);
	//	for (int i=0; i<RC5_NUM_SUBKEYS; ++i) {
	//		LOGi("RC5 key[%i] = %u", i, _rc5SubKeys[i]);
	//	}
	return true;
}

// See https://en.wikipedia.org/wiki/RC5
bool RC5::decrypt(uint16_t* inBuf, uint16_t inBufSize, uint16_t* outBuf, uint16_t outBufSize) {
	if (inBufSize < RC5_BLOCK_SIZE || outBufSize < RC5_BLOCK_SIZE) {
		return false;
	}
	uint16_t a = inBuf[0];
	uint16_t b = inBuf[1];
	uint16_t sum;
	for (uint16_t i = RC5_ROUNDS; i > 0; --i) {
		sum = b - _subKeys[2 * i + 1];
		b   = ROTR_16BIT(sum, a % 16) ^ a;
		sum = a - _subKeys[2 * i];
		a   = ROTR_16BIT(sum, b % 16) ^ b;
	}
	outBuf[0] = a - _subKeys[0];
	outBuf[1] = b - _subKeys[1];
	return true;
}
