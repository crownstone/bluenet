/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <structs/cs_PacketsInternal.h>

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
	cs_ret_code_t encryptCtr(cs_data_t key, cs_data_t prefix, cs_data_t input, cs_data_t output, cs_buffer_size_t& writtenSize);


private:
	// This class is singleton, make constructor private.
	AES();

	// This class is singleton, deny implementation
	AES(AES const&);

	// This class is singleton, deny implementation
	void operator=(AES const &);


};

