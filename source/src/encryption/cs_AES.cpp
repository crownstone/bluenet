/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <encryption/cs_AES.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#include <algorithm> // for std::min

#if ENCRYPTION_KEY_LENGTH != SOC_ECB_KEY_LENGTH
	#error "AES key size mismatch"
#endif

#if AES_BLOCK_SIZE != SOC_ECB_CLEARTEXT_LENGTH || AES_BLOCK_SIZE != SOC_ECB_CIPHERTEXT_LENGTH
	#error "AES block size mismatch"
#endif

#define LOGAesDebug LOGnone
#define LOGAesVerbose LOGnone

AES::AES() {

}

cs_ret_code_t AES::encryptEcb(cs_data_t key, cs_data_t prefix, cs_data_t input, cs_data_t output, cs_buffer_size_t& writtenSize) {
	uint32_t errCode;
	uint8_t softdeviceEnabled;
	errCode = sd_softdevice_is_enabled(&softdeviceEnabled);
	if (errCode != NRF_SUCCESS || !softdeviceEnabled) {
		LOGw("Softdevice not enabled!");
		return ERR_NOT_INITIALIZED;
	}

	if (key.len < sizeof(_block.key)) {
		LOGw("key too short");
		return ERR_WRONG_PARAMETER;
	}

	uint32_t totalInputSize = prefix.len + input.len;
	uint32_t outputSize = CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(totalInputSize, AES_BLOCK_SIZE);
	uint16_t numBlocks = outputSize / AES_BLOCK_SIZE;
	writtenSize = 0;

	if (totalInputSize == 0) {
		LOGw("nothing to encrypt");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	if (outputSize > output.len) {
		LOGw("output buffer too small");
		return ERR_BUFFER_TOO_SMALL;
	}

	// Set the key.
	memcpy(_block.key, key.data, sizeof(_block.key));

	// how much has been written to current block so far.
	cs_buffer_size_t blockWrittenSize = 0;

	// how much to write from current buffer.
	cs_buffer_size_t writeSize;

	// how much has been read from prefix buffer.
	cs_buffer_size_t prefixReadSize = 0;

	// how much has been read from input buffer.
	cs_buffer_size_t inputReadSize = 0;

	for (int i = 0; i < numBlocks; ++i) {
		blockWrittenSize = 0;

		// Prefix data.
		writeSize = std::min(prefix.len - prefixReadSize, AES_BLOCK_SIZE - blockWrittenSize);
		memcpy(_block.cleartext + blockWrittenSize, prefix.data + prefixReadSize, writeSize);
		prefixReadSize += writeSize;
		blockWrittenSize += writeSize;
		LOGAesVerbose("write prefix from=%u to=%u size=%u", prefix.data + prefixReadSize, _block.cleartext + blockWrittenSize, writeSize);

		// Input data.
		writeSize = std::min(input.len - inputReadSize, AES_BLOCK_SIZE - blockWrittenSize);
		memcpy(_block.cleartext + blockWrittenSize, input.data + inputReadSize, writeSize);
		inputReadSize += writeSize;
		blockWrittenSize += writeSize;
		LOGAesVerbose("write input from=%u to=%u size=%u", input.data + inputReadSize, _block.cleartext + blockWrittenSize, writeSize);

		// Zero padding.
		writeSize = AES_BLOCK_SIZE - blockWrittenSize;
		memset(_block.cleartext + blockWrittenSize, 0, writeSize);
		blockWrittenSize += writeSize;
		LOGAesVerbose("write zero padding to=%u size=%u", _block.cleartext + blockWrittenSize, writeSize);

		// Encrypts the cleartext and puts it in ciphertext.
		errCode = sd_ecb_block_encrypt(&_block);
		APP_ERROR_CHECK(errCode);

		// Copy to output
		memcpy(output.data + writtenSize, _block.ciphertext, AES_BLOCK_SIZE);
		writtenSize += AES_BLOCK_SIZE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t AES::encryptCtr(cs_data_t key, cs_data_t nonce, cs_data_t prefix, cs_data_t input, cs_data_t output, cs_buffer_size_t& writtenSize, uint8_t blockCtr) {
	return ctr(key, nonce, prefix, input, cs_data_t(), output, writtenSize, blockCtr);
}

cs_ret_code_t AES::decryptCtr(cs_data_t key, cs_data_t nonce, cs_data_t input, cs_data_t prefix, cs_data_t output, cs_buffer_size_t& writtenSize, uint8_t blockCtr) {
	if (input.len % AES_BLOCK_SIZE) {
		LOGw(STR_ERR_MULTIPLE_OF_16);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	return ctr(key, nonce, cs_data_t(), input, prefix, output, writtenSize);
}

cs_ret_code_t AES::ctr(cs_data_t key, cs_data_t nonce, cs_data_t inputPrefix, cs_data_t input, cs_data_t outputPrefix, cs_data_t output, cs_buffer_size_t& writtenSize, uint8_t blockCtr) {
	LOGAesVerbose("CTR key=%u nonce=%u inputPrefix=%u input=%u outputPrefix=%u output=%u ctr=%u",
			key.data,
			nonce.data,
			inputPrefix.data,
			input.data,
			outputPrefix.data,
			output.data,
			blockCtr
			);

	uint32_t errCode;
	uint8_t softdeviceEnabled;
	errCode = sd_softdevice_is_enabled(&softdeviceEnabled);
	if (errCode != NRF_SUCCESS || !softdeviceEnabled) {
		LOGw("Softdevice not enabled.");
		return ERR_NOT_INITIALIZED;
	}

	if (key.len < sizeof(_block.key)) {
		LOGw("Key too short.");
		return ERR_WRONG_PARAMETER;
	}

	uint32_t totalInputSize = inputPrefix.len + input.len;
	uint32_t outputSize = CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(totalInputSize, AES_BLOCK_SIZE);
	uint16_t numBlocks = outputSize / AES_BLOCK_SIZE;
	writtenSize = 0;

	if (numBlocks > 255) {
		LOGw("Too many blocks.");
		return ERR_NO_SPACE;
	}

	if (totalInputSize == 0) {
		LOGw("Nothing to encrypt.");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	if (outputSize > outputPrefix.len + output.len) {
		LOGw("Output buffer too small: required=%u prefix=%u output=%u", outputSize, outputPrefix.len, output.len);
		return ERR_BUFFER_TOO_SMALL;
	}

	// Set the key.
	memcpy(_block.key, key.data, sizeof(_block.key));

	// Prepare the IV (concatenation of nonce and counter) and put it in the clear text.
	memset(_block.cleartext, 0, AES_BLOCK_SIZE);
	memcpy(_block.cleartext, nonce.data, nonce.len);

	// how much has been written to current block so far.
	cs_buffer_size_t blockWrittenSize = 0;

	// how much to write from current buffer.
	cs_buffer_size_t writeSize;

	// how much has been read from input prefix buffer.
	cs_buffer_size_t prefixReadSize = 0;

	// how much has been read from input buffer.
	cs_buffer_size_t inputReadSize = 0;

	// how much has been written to output prefix buffer.
	cs_buffer_size_t prefixWrittenSize = 0;

	for (uint8_t i = 0; i < numBlocks; ++i) {

		// Put the counter in the IV.
		_block.cleartext[AES_BLOCK_SIZE - 1] = i + blockCtr;

		// Encrypts the cleartext and puts it in ciphertext.
//		_log(SERIAL_DEBUG, false, "key: ");
//		BLEutil::printArray(_block.key, sizeof(_block.key), SERIAL_DEBUG);
//		_log(SERIAL_DEBUG, false, "cleartext: ");
//		BLEutil::printArray(_block.cleartext, sizeof(_block.cleartext), SERIAL_DEBUG);
		errCode = sd_ecb_block_encrypt(&_block);
		APP_ERROR_CHECK(errCode);
//		_log(SERIAL_DEBUG, false, "cipher: ");
//		BLEutil::printArray(_block.ciphertext, sizeof(_block.ciphertext), SERIAL_DEBUG);

		///////////////////////////////////////////////////////////////////
		// XOR the ciphertext with the data to finish encrypting the block.
		///////////////////////////////////////////////////////////////////
		blockWrittenSize = 0;

		// Prefix data.
		writeSize = std::min(inputPrefix.len - prefixReadSize, AES_BLOCK_SIZE - blockWrittenSize);
		LOGAesVerbose("write prefix from=%u to=%u size=%u", inputPrefix.data + prefixReadSize, _block.cleartext + blockWrittenSize, writeSize);
		for (uint8_t j = 0; j < writeSize; j++) {
			_block.ciphertext[blockWrittenSize] ^= inputPrefix.data[prefixReadSize];
			++prefixReadSize;
			++blockWrittenSize;
		}

		// Input data.
		writeSize = std::min(input.len - inputReadSize, AES_BLOCK_SIZE - blockWrittenSize);
		LOGAesVerbose("write input from=%u to=%u size=%u", input.data + inputReadSize, _block.cleartext + blockWrittenSize, writeSize);
		for (uint8_t j = 0; j < writeSize; j++) {
			_block.ciphertext[blockWrittenSize] ^= input.data[inputReadSize];
			++inputReadSize;
			++blockWrittenSize;
		}

		// Zero padding.
		writeSize = AES_BLOCK_SIZE - blockWrittenSize;
		LOGAesVerbose("write zero padding to=%u size=%u", _block.cleartext + blockWrittenSize, writeSize);
		for (uint8_t j = 0; j < writeSize; j++) {
			_block.ciphertext[blockWrittenSize] ^= 0;
			++blockWrittenSize;
		}

		//////////////////
		// Copy to output.
		//////////////////
		blockWrittenSize = 0;

		writeSize = std::min(outputPrefix.len - prefixWrittenSize, AES_BLOCK_SIZE - blockWrittenSize);
		LOGAesVerbose("write output prefix from=%u to=%u size=%u", _block.ciphertext + blockWrittenSize, outputPrefix.data + prefixWrittenSize, writeSize);
		memcpy(outputPrefix.data + prefixWrittenSize, _block.ciphertext + blockWrittenSize, writeSize);
		prefixWrittenSize += writeSize;
		blockWrittenSize += writeSize;

		writeSize = AES_BLOCK_SIZE - blockWrittenSize;
		LOGAesVerbose("write output from=%u to=%u size=%u", _block.ciphertext + blockWrittenSize, output.data + writtenSize, writeSize);
		memcpy(output.data + writtenSize, _block.ciphertext + blockWrittenSize, writeSize);
		blockWrittenSize += writeSize;
		writtenSize += writeSize;
	}
	return ERR_SUCCESS;

}
