/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_Packets.h>

#define RC5_ROUNDS 12
#define RC5_NUM_SUBKEYS (2 * (RC5_ROUNDS + 1))  // t = 2(r+1) - the number of round subkeys required.
#define RC5_KEYLEN 16
#define RC5_BLOCK_SIZE 4  // Size in bytes of an RC5 block.

/**
 * Class that implements RC5 encryption.
 *
 * - Block size is 32 bit (so 16 bit words).
 * - Only has decrypt method implemented, but encrypt would be possible too.
 */
class RC5 {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static RC5& getInstance() {
		static RC5 instance;
		return instance;
	}

	/**
	 * Initialize the class, reads from State.
	 */
	void init();

	/**
	 * Decrypt data with RC5.
	 *
	 * @param[in]  inBuf               Input buffer with the encrypted data.
	 * @param[in]  inBufSize           Size of the input buffer in bytes.
	 * @param[out] outBuf              Output buffer to decrypt to.
	 * @param[in]  outBufSize          Size of the output buffer in bytes.
	 */
	bool decrypt(uint16_t* inBuf, uint16_t inBufSize, uint16_t* outBuf, uint16_t outBufSize);

private:
	// This class is singleton, make constructor private.
	RC5();

	// This class is singleton, deny implementation
	RC5(RC5 const&);

	// This class is singleton, deny implementation
	void operator=(RC5 const&);

	/**
	 * The round subkey words.
	 */
	uint16_t _subKeys[RC5_NUM_SUBKEYS];

	/**
	 * Expands the key of given access level.
	 */
	bool initKey(EncryptionAccessLevel accessLevel);

	/**
	 * Expands key into round subkey words, which are cached.
	 */
	bool prepareKey(uint8_t* key, uint8_t keyLength);
};
