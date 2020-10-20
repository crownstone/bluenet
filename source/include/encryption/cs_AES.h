/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <structs/cs_PacketsInternal.h>

/**
 * AES block size.
 *
 * 16 byte size, just like SOC_ECB_CLEARTEXT_LENGTH and SOC_ECB_CIPHERTEXT_LENGTH.
 */
#define AES_BLOCK_SIZE 16

class AES {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static AES& getInstance() {
		static AES instance;
		return instance;
	}

	/**
	 * Initialize the class, reads from State.
	 */
	void init();

	/**
	 * Encrypt data with given key in ECB mode.
	 *
	 * The data that's encrypted is a concatenation of prefix and input.
	 *
	 * @param[in]  key                 Key to encrypt with.
	 * @param[in]  prefix              Extra data to put before the input data. Can be skipped by passing a null pointer and data size 0.
	 * @param[in]  input               Input data to be encrypted.
	 * @param[out] output              Buffer to encrypt to. Can be the same as input, as long as: output pointer >= input pointer + prefix size.
	 * @param[out] writtenSize         How many bytes are written to output.
	 * @return                         Return code.
	 */
	cs_ret_code_t encryptEcb(cs_data_t key, cs_data_t prefix, cs_data_t input, cs_data_t output, cs_buffer_size_t& writtenSize);

	/**
	 * Encrypt data with given key in CTR mode.
	 *
	 * The data that's encrypted is a concatenation of prefix and input.
	 *
	 * @param[in]  key                 Key to encrypt with.
	 * @param[in]  nonce               Nonce to use for encryption.
	 * @param[in]  prefix              Extra data to put before the input data.
	 * @param[in]  input               Input data to be encrypted.
	 * @param[out] output              Buffer to encrypt to. Can be the same as input, as long as: output pointer >= input pointer + prefix size.
	 * @param[out] writtenSize         How many bytes are written to output.
	 * @return                         Return code.
	 */
	cs_ret_code_t encryptCtr(cs_data_t key, cs_data_t nonce, cs_data_t prefix, cs_data_t input, cs_data_t output, cs_buffer_size_t& writtenSize);

	/**
	 * Decrypt data with given key in CTR mode.
	 *
	 * The decrypted data is a concatenation of prefix and output.
	 *
	 * @param[in]  key                 Key to encrypt with.
	 * @param[in]  nonce               Nonce to use for decryption.
	 * @param[in]  input               Input data to be encrypted.
	 * @param[out] prefix              Buffer to encrypt to, before the output buffer. Can be the same as input.
	 * @param[out] output              Buffer to encrypt to. Can be the same as input, as long as: output pointer >= input pointer + prefix size.
	 * @return                         Return code.
	 */
	cs_ret_code_t decryptCtr(cs_data_t key, cs_data_t nonce, cs_data_t input, cs_data_t prefix, cs_data_t output);


private:
	// This class is singleton, make constructor private.
	AES();

	// This class is singleton, deny implementation
	AES(AES const&);

	// This class is singleton, deny implementation
	void operator=(AES const &);

	/**
	 * Encrypt or decrypt data with given key in CTR mode.
	 *
	 * @param[in]  key                 Key to encrypt with.
	 * @param[in]  nonce               Nonce to use for encryption.
	 * @param[in]  inputPrefix         Extra data to put before the input data.
	 * @param[in]  input               Input data to be encrypted.
	 * @param[in]  outputPrefix        Extra data to be written to before writing to the output buffer.
	 * @param[out] output              Buffer to encrypt to. Can be the same as input, as long as: output pointer + output prefix size >= input pointer + input prefix size.
	 * @param[out] writtenSize         How many bytes are written to output.
	 * @return                         Return code.
	 */
	cs_ret_code_t ctr(cs_data_t key, cs_data_t nonce, cs_data_t inputPrefix, cs_data_t input, cs_data_t outputPrefix, cs_data_t output, cs_buffer_size_t& writtenSize);

	/**
	 * Struct with key, and single block of encrypted and decypted data.
	 */
	nrf_ecb_hal_data_t _block __attribute__ ((aligned (4)));

};

