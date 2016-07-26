/*
 * cs_EncryptedPackage.cpp
 *
 *  Created on: Jul 19, 2016
 *      Author: alex
 */



#include <structs/cs_EncryptedPackage.h>
#include <storage/cs_Settings.h>
#include <drivers/cs_RNG.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>


/**
 * cleanup of the memory allocated in the init method.
 */
EncryptedPackage::~EncryptedPackage() {
	delete _block;
}


/**
 * Allocate memory for all required fields
 */
void EncryptedPackage::init() {
	_block = new nrf_ecb_hal_data_t __aligned(4);
	_sessionNonce = new uint8_t[5];
	_initialized = true;
	EventDispatcher::getInstance().addListener(this);
}

/**
 * make sure we create a new nonce for each connection
 */
void EncryptedPackage::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_BLE_CONNECT:
		_generateSessionNonce();
		break;
	}
}

/**
 * Allow characteristics to get the sessionNonce. Length of the sessionNonce is 5 bytes.
 */
uint8_t* EncryptedPackage::getSessionNonce() {
	return _sessionNonce;
}



/**
 * This method does not use the AES CTR method but straight ECB.
 * It encrypts the block with the GUEST key.
 */
void EncryptedPackage::encryptAdvertisement(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength) {
	// must be 1 block.
	if (dataLength != SOC_ECB_CLEARTEXT_LENGTH || targetLength != SOC_ECB_CIPHERTEXT_LENGTH) {
		LOGe("The length of the data and/or target buffers is not 16 bytes. Cannot encrypt advertisement.");
		return;
	}

	// set the guest key as active encryption key
	if (_checkAndSetKey(GUEST)) {
		uint32_t err_code;

		// insert the encryption key into the block
		memcpy(_block->cleartext, data, SOC_ECB_CLEARTEXT_LENGTH);

		// encrypts the cleartext and puts it in ciphertext
		err_code = sd_ecb_block_encrypt(_block);
		APP_ERROR_CHECK(err_code);

		// copy result into target.
		memcpy(target, _block->ciphertext, SOC_ECB_CIPHERTEXT_LENGTH);
	}
	else {
		LOGe("Guest key is not set. Cannot encrypt advertisement.");
	}

}

/**
 * This method encrypts the data and places the result into the target struct.
 * It is IMPORTANT that the target HAS a bufferLength that is dividable by 16 and not 0.
 * The buffer of the target NEEDS to be allocated for the bufferLength
 */
void EncryptedPackage::encrypt(uint8_t* data, uint16_t dataLength, encrypted_package_t& target) {
	// check if the userLevel has been set
	if (_checkAndSetKey(target.userLevel) == false)
		return;

	// generate the nonce and copy it into the class block
	_generateNewNonce(target.nonce);

	_createIV(_block->cleartext, target.nonce);

	if (_performAES128CTR(data, dataLength, target.buffer, target.bufferLength) == false) {
		LOGe("Error while encrypting");
	}
}

void EncryptedPackage::decrypt(encrypted_package_t& data, uint8_t* target, uint16_t targetLength) {
	// check if the userLevel has is correct
	if (_checkAndSetKey(data.userLevel) == false)
		return;

	if (data.bufferLength == targetLength) {
		_createIV(_block->cleartext, data.nonce);

		if (_performAES128CTR(data.buffer, data.bufferLength, target, targetLength) == false) {
			LOGe("Error while decrypting.");
		}

		//TODO: validate
	}
}


/**
 * This is where the magic happens. There are a few things that have to be done before this method is called:
 * 1) The correct key has to be set in the member variable _aesKey
 * 2) The correct Nonce has to be set in the member variable _nonce
 * Since we use the AES128 CTR method, the encryption and decryption only really differ on what we use in the XOR phase.
 *
 * The outputLength should be larger than 0 and a multiple of 16 or this will return false.
 * It is possible that the input is smaller than the output, in this case it will be zero padded.
 *
 * For decrypting we expect the input to have the same length as the output. This check is in the decrypt method.
 */
bool EncryptedPackage::_performAES128CTR(uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength) {
	if (_validateBlockLength(outputLength) == false) {
		return false;
	}

	uint32_t err_code;

	// amount of blocks to loop over
	uint16_t blockCount = outputLength / SOC_ECB_CIPHERTEXT_LENGTH;

	// shift is the position in the data that is being encrypted.
	uint16_t shift = 0;

	// encrypt all blocks using AES 128 CTR block mode.
	for (uint8_t counter = 0; counter < blockCount; counter++) {
		// prepare the nonce for the next step by concatenating the nonce with the counter
		_block->cleartext[SOC_ECB_CLEARTEXT_LENGTH-1] = counter;

		// encrypts the cleartext and puts it in ciphertext
		err_code = sd_ecb_block_encrypt(_block);
		APP_ERROR_CHECK(err_code);

		// calculate the location in the byte array that we are using
		shift = counter * SOC_ECB_CIPHERTEXT_LENGTH;

		// XOR the ciphertext with the data to finish encrypting the block.
		for (uint8_t i = 0; i < SOC_ECB_CIPHERTEXT_LENGTH; i++) {
			if (i + shift < inputLength)
				_block->ciphertext[i] ^= input[i + shift];
			else
				_block->ciphertext[i] ^= 0; // zero padding the data to fit in the block.
		}

		// copy the encrypted block over to the target
		// the buffer is shifted by the block count (output + shift --> pointer to output[shift])
		memcpy(output + shift, _block->ciphertext, SOC_ECB_CIPHERTEXT_LENGTH);
	}

	return true;
}


/**
 * Check if the key that we need is set in the memory and if so, set it into the encryption block
 */
bool EncryptedPackage::_checkAndSetKey(uint8_t userLevel) {
	// check if the userLevel is set.
	if (userLevel == NOT_SET) {
		LOGe("The userLevel is not set in the struct.");
		return false;
	}

	ConfigurationTypes keyConfigType;
	switch (userLevel) {
	case ADMIN:
		keyConfigType = CONFIG_KEY_OWNER;
		break;
	case USER:
		keyConfigType = CONFIG_KEY_MEMBER;
		break;
	case GUEST:
		keyConfigType = CONFIG_KEY_GUEST;
		break;
	default:
		LOGe("Provided userLevel does not exist (level is not admin, user or guest)");
		return false;
	}

	// get the key from the storage
	Settings::getInstance().get(keyConfigType, _block->key);
	return true;
}

/**
 * This method will fill the buffer with 3 random bytes. These are included in the message
 */
void EncryptedPackage::_generateNewNonce(uint8_t* target) {
	RNG::fillBuffer(target, PACKET_NONCE_LENGTH);
}

/**
 * This method will fill the buffer with 5 random bytes. This is done on connect and is only valid once.
 */
void EncryptedPackage::_generateSessionNonce() {
	if (_initialized) {
		RNG::fillBuffer(_sessionNonce, SESSION_NONCE_LENGTH);
	}
}

/**
 * The Nonce of the CTR method is made up of 8 random bytes:
 * 	- 3 from the packet nonce
 * 	- 5 from the session nonce
 * We use the term IV (initialization vector) for this because we have enough Nonce terms floating around.
 */
void EncryptedPackage::_createIV(uint8_t* target, uint8_t* nonce) {
	// Set the nonce to zero to ensure it is consistent.
	// Since we only have 1 uint8_t as counter doing this once is enough.
	memset(target, 0x00, SOC_ECB_CLEARTEXT_LENGTH);
	// half the text is nonce, the other half is a counter
	memcpy(target, nonce, PACKET_NONCE_LENGTH);
	// copy the session nonce over into the target
	memcpy(target + PACKET_NONCE_LENGTH, _sessionNonce, SESSION_NONCE_LENGTH);
}


/**
 * Verify if the block lengh is correct
 */
bool EncryptedPackage::_validateBlockLength(uint16_t length) {
	if (length % SOC_ECB_CLEARTEXT_LENGTH != 0 || length == 0) {
		LOGe("Length of block does not match the expected length (16 Bytes).");
		return false;
	}
	return true;
}

