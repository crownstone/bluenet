/*
 * cs_EncryptedPackage.cpp
 *
 *  Created on: Jul 19, 2016
 *      Author: alex
 */

#include <processing/cs_EncryptionHandler.h>
#include <storage/cs_Settings.h>
#include <drivers/cs_Serial.h>
#include <ble/cs_Stack.h>
#include <events/cs_EventDispatcher.h>

//#define TESTING_ENCRYPTION true


void EncryptionHandler::init() {
	_defaultValidationKey.b = DEFAULT_SESSION_KEY;
	EventDispatcher::getInstance().addListener(this);
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);
}

uint16_t EncryptionHandler::calculateEncryptionBufferLength(uint16_t inputLength, EncryptionType encryptionType) {
	if (encryptionType == CTR || encryptionType == CTR_CAFEBABE) {
		// add the validation padding length to the raw data length
		uint16_t requiredLength = inputLength + VALIDATION_NONCE_LENGTH;

		uint16_t blockOverflow = requiredLength % SOC_ECB_CLEARTEXT_LENGTH;

		// check if we need an extra block
		requiredLength += blockOverflow > 0 ? SOC_ECB_CLEARTEXT_LENGTH - blockOverflow : 0;

		// add overhead
		requiredLength += PACKET_NONCE_LENGTH + USER_LEVEL_LENGTH;

		return requiredLength;
	}
	else {
		return SOC_ECB_CIPHERTEXT_LENGTH;
	}
}


uint16_t EncryptionHandler::calculateDecryptionBufferLength(uint16_t encryptedPacketLength) {
	return encryptedPacketLength - VALIDATION_NONCE_LENGTH - PACKET_NONCE_LENGTH - USER_LEVEL_LENGTH;
}

void EncryptionHandler::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_BLE_CONNECT:
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED))
			_generateSessionNonce();
		break;
	}
}


uint8_t* EncryptionHandler::getSessionNonce() {
	return _sessionNonce;
}


void EncryptionHandler::closeConnectionAuthenticationFailure() {
	Nrf51822BluetoothStack::getInstance().disconnect();
}


bool EncryptionHandler::allowedToWrite() {
	return Nrf51822BluetoothStack::getInstance().isDisconnecting();
}


/**
 * This method does not use the AES CTR method but straight ECB.
 * It encrypts the block with the GUEST key.
 */
bool EncryptionHandler::_encryptECB(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType) {
	// total length must be exactly 1 block.
	if (targetLength != SOC_ECB_CIPHERTEXT_LENGTH) {
		LOGe("Invalid target buffer length %d %d",dataLength, targetLength);
		return false;
	}

	if ((encryptionType == ECB_GUEST_CAFEBABE && dataLength + DEFAULT_SESSION_KEY_LENGTH > SOC_ECB_CLEARTEXT_LENGTH) ||
			(encryptionType == ECB_GUEST && dataLength > SOC_ECB_CLEARTEXT_LENGTH)) {
		LOGe("Invalid input buffer length");
		return false;
	}

	if (_checkAndSetKey(userLevel) == false)
		return false;

	// set the guest key as active encryption key
	uint32_t err_code;

	// set the cleartext to 0x0 so we automatically zeropad the cleartext
	memset(_block.cleartext, 0x0, SOC_ECB_CLEARTEXT_LENGTH);

	if (encryptionType == ECB_GUEST_CAFEBABE) {
		// copy the validation key into the cleartext
		memcpy(_block.cleartext, _defaultValidationKey.a, DEFAULT_SESSION_KEY_LENGTH);

		// after which, copy the data into the cleartext
		memcpy(_block.cleartext + DEFAULT_SESSION_KEY_LENGTH, data, dataLength);
	}
	else {
		// copy the data over into the cleartext
		memcpy(_block.cleartext, data, dataLength);
	}

	// encrypts the cleartext and puts it in ciphertext
	err_code = sd_ecb_block_encrypt(&_block);
	APP_ERROR_CHECK(err_code);

	// copy result into target.
	memcpy(target, _block.ciphertext, SOC_ECB_CIPHERTEXT_LENGTH);

	return true;
}


bool EncryptionHandler::encryptMesh(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength) {
	return encrypt(data, dataLength, target, targetLength, ADMIN, CTR_CAFEBABE);
}


bool EncryptionHandler::encrypt(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType) {
	if (encryptionType == CTR || encryptionType == CTR_CAFEBABE) {
		return _prepareEncryptCTR(data,dataLength,target,targetLength,userLevel,encryptionType);
	}
	else {
		return _encryptECB(data,dataLength,target,targetLength,userLevel,encryptionType);
	}
}

bool EncryptionHandler::_prepareEncryptCTR(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType) {
	// check if the userLevel has been set
	if (_checkAndSetKey(userLevel) == false)
		return false;

	uint16_t targetNetLength = targetLength - _overhead;

	// check if the input would still fit if the nonce is added.
	if (dataLength + VALIDATION_NONCE_LENGTH > targetNetLength) {
		LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
	//		LOGe("Length of input block would be larger than the output block if we add the validation nonce.");
		return false;
	}

	// generate a Nonce for this session and write it to the first 3 bytes of the target.
	_generateNonceInTarget(target);

	// write the userLevel to the target
	target[PACKET_NONCE_LENGTH] = uint8_t(userLevel);

	// create the IV
	_createIV(_block.cleartext, target, encryptionType);

	if (_encryptCTR(data, dataLength, target+_overhead, targetNetLength, encryptionType) == false) {
		LOGe("Error while encrypting");
		return false;
	}

	return true;
}



bool EncryptionHandler::decrypt(uint8_t* encryptedDataPacket, uint16_t encryptedDataPacketLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel& levelOfPackage, EncryptionType encryptionType) {
	if (!(encryptionType == CTR || encryptionType == CTR_CAFEBABE)) {
		LOGe("Cannot decrypt ECB");
		return false;
	}

	// check if the size of the encrypted data packet makes sense: min 1 block and overhead
	if (encryptedDataPacketLength < SOC_ECB_CIPHERTEXT_LENGTH + _overhead) {
		LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
//		LOGe("Encryted data packet is smaller than the minimum possible size of a block and overhead (20 bytes).");
		return false;
	}

	// check if the userLevel has is correct
	if (_checkAndSetKey(encryptedDataPacket[PACKET_NONCE_LENGTH]) == false)
		return false;

	// tell the reference the access level of the packet
	switch (encryptedDataPacket[PACKET_NONCE_LENGTH]) {
		case 0:
			levelOfPackage = ADMIN; break;
		case 1:
			levelOfPackage = MEMBER; break;
		case 2:
			levelOfPackage = GUEST; break;
		case 100:
			levelOfPackage = SETUP; break;
	}

	// the actual encrypted part is after the overhead
	uint16_t sourceNetLength = encryptedDataPacketLength - _overhead;

	// setup the IV
	_createIV(_block.cleartext, encryptedDataPacket, encryptionType);

	if (_decryptCTR(encryptedDataPacket + _overhead, sourceNetLength, target, targetLength, encryptionType) == false) {
		LOGe("Error while decrypting");
		return false;
	}

	return true;
}

/**
 * This is where the magic happens. There are a few things that have to be done before this method is called:
 * 1) The correct key has to be set in the _block.key
 * 2) The correct Nonce has to be set in the _block.clearText
 *
 * The input length should be a multiple of 16 bytes.
 * It is possible that the target is smaller than the input, in this case the remainder is discarded.
 *
 */
bool EncryptionHandler::_decryptCTR(uint8_t* input, uint16_t inputLength, uint8_t* target, uint16_t targetLength, EncryptionType encryptionType) {
	if (_validateBlockLength(inputLength) == false) {
		LOGe(STR_ERR_MULTIPLE_OF_16);
//		LOGe("Length of input block does not match the expected length (N*16 Bytes).");
		return false;
	}
	uint32_t err_code;

	// amount of blocks to loop over
	uint8_t blockOverflow = (targetLength + VALIDATION_NONCE_LENGTH) %  SOC_ECB_CIPHERTEXT_LENGTH;
	uint16_t blockCount = (targetLength + VALIDATION_NONCE_LENGTH) / SOC_ECB_CIPHERTEXT_LENGTH;
	blockCount += (blockOverflow > 0) ? 1 : 0;

	// variables to keep track of where data is written
	uint16_t inputReadIndex = 0;
	uint16_t writtenToTarget = 0;
	uint16_t targetLengthLeftToWrite = targetLength;
	uint16_t maxIterationWriteAmount;

	// encrypt all blocks using AES 128 CTR block mode.
	for (uint8_t counter = 0; counter < blockCount; counter++) {
		// prepare the IV for the next step by concatenating the nonce with the counter
		_block.cleartext[SOC_ECB_CLEARTEXT_LENGTH-1] = counter;

		// encrypts the cleartext and puts it in ciphertext
		err_code = sd_ecb_block_encrypt(&_block);
		APP_ERROR_CHECK(err_code);

		// XOR the ciphertext with the data to finish encrypting the block.
		for (uint8_t i = 0; i < SOC_ECB_CIPHERTEXT_LENGTH; i++) {
			inputReadIndex = i + counter * SOC_ECB_CIPHERTEXT_LENGTH;
			if (inputReadIndex < inputLength)
				_block.ciphertext[i] ^= input[inputReadIndex];
			else {
				LOGe("Length mismatch %d", inputReadIndex);
				return false;
			}
		}


		// check the validation nonce
		if (counter == 0) {
			if (_validateDecryption(_block.ciphertext, encryptionType) == false)
				return false;

			// the first iteration we can only write 12 relevant bytes to the target because the first 4 are the nonce.
			maxIterationWriteAmount = SOC_ECB_CIPHERTEXT_LENGTH - VALIDATION_NONCE_LENGTH;
			if (targetLengthLeftToWrite < maxIterationWriteAmount)
				maxIterationWriteAmount = targetLengthLeftToWrite;

			// we offset the pointer of the cipher text by the length of the validation length
			memcpy(target, _block.ciphertext + VALIDATION_NONCE_LENGTH, maxIterationWriteAmount);

		}
		else {
			// after the first iteration, we can write the full 16 bytes.
			maxIterationWriteAmount = SOC_ECB_CIPHERTEXT_LENGTH;
			if (targetLengthLeftToWrite < maxIterationWriteAmount)
				maxIterationWriteAmount = targetLengthLeftToWrite;

			memcpy(target + writtenToTarget, _block.ciphertext, maxIterationWriteAmount);
		}

		// update the variables that keep track of the writing.
		targetLengthLeftToWrite -= maxIterationWriteAmount;
		writtenToTarget += maxIterationWriteAmount;
	}

	if (targetLengthLeftToWrite != 0) {
		LOGe("Not all decrypted data has been written! %d", targetLengthLeftToWrite);
		return false;
	}

	return true;
}


/**
 * Checks if the first 4 bytes of the decrypted buffer match the session nonce or the cafebabe if useSessionNonce is false.
 */
bool EncryptionHandler::_validateDecryption(uint8_t* buffer, EncryptionType encryptionType) {
	for (uint8_t i = 0; i < VALIDATION_NONCE_LENGTH; i++) {
		if (encryptionType == CTR_CAFEBABE) {
			if (buffer[i] != _defaultValidationKey.a[i]) {
				LOGe("Nonce mismatch");
//				LOGe("Crownstone session nonce does not match 0xcafebabe in packet. Can not decrypt.");
				return false;
			}
		}
		else {
			if (buffer[i] != _sessionNonce[i]) {
				LOGe("Nonce mismatch");
//				LOGe("Crownstone session nonce does not match session nonce in packet. Can not decrypt.");
				return false;
			}
		}
	}

	return true;
}




/**
 * This is where the magic happens. There are a few things that have to be done before this method is called:
 * 1) The correct key has to be set in the _block.key
 * 2) The correct Nonce has to be set in the _block.clearText
 *
 * The outputLength should be larger than 0 and a multiple of 16 or this will return false.
 * It is possible that the input is smaller than the output, in this case it will be zero padded.
 *
 * The validation nonce is automatically added to the input.
 */
bool EncryptionHandler::_encryptCTR(uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength, EncryptionType encryptionType) {
	if (_validateBlockLength(outputLength) == false) {
		LOGe(STR_ERR_MULTIPLE_OF_16);
//		LOGe("Length of output block does not match the expected length (N*16 Bytes).");
		return false;
	}

	uint32_t err_code;

	// amount of blocks to loop over
	uint16_t blockCount = outputLength / SOC_ECB_CIPHERTEXT_LENGTH;

	// shift is the position in the data that is being encrypted.
	uint16_t shift = 0;

	uint32_t inputReadIndex = 0;

#ifdef TESTING_ENCRYPTION
	for (uint8_t i = 0; i < SOC_ECB_KEY_LENGTH; i++) {
		LOGi("key: %d", _block.key[i]);
	}
#endif

	// encrypt all blocks using AES 128 CTR block mode.
	for (uint8_t counter = 0; counter < blockCount; counter++) {
		// prepare the nonce for the next step by concatenating the nonce with the counter
		_block.cleartext[SOC_ECB_CLEARTEXT_LENGTH-1] = counter;

#ifdef TESTING_ENCRYPTION
	for (uint8_t i = 0; i < SOC_ECB_CLEARTEXT_LENGTH; i++) {
		LOGi("IV: %d", _block.cleartext[i]);
	}
#endif
		// encrypts the cleartext and puts it in ciphertext
		err_code = sd_ecb_block_encrypt(&_block);
		APP_ERROR_CHECK(err_code);

		// calculate the location in the byte array that we are using
		shift = counter * SOC_ECB_CIPHERTEXT_LENGTH;

		// XOR the ciphertext with the data to finish encrypting the block.
		for (uint8_t i = 0; i < SOC_ECB_CIPHERTEXT_LENGTH; i++) {
			// if we are at the first block, we will add the validation nonce (VN) to the first 4 bytes
			if (shift == 0 && i < VALIDATION_NONCE_LENGTH) {
				if (encryptionType == CTR_CAFEBABE) {
					// in case we need a checksum but not a session, we use 0xcafebabe
					_block.ciphertext[i] ^= _defaultValidationKey.a[i];

				}
				else {
					// this will make sure the first 4 bytes of the payload are the validation nonce
					_block.ciphertext[i] ^= _sessionNonce[i];
				}
				continue; // continue to next iteration to short circuit the loop
			}
			inputReadIndex = i + shift - VALIDATION_NONCE_LENGTH;

			if (inputReadIndex < inputLength)
				_block.ciphertext[i] ^= input[inputReadIndex];
			else
				_block.ciphertext[i] ^= 0; // zero padding the data to fit in the block.
		}


		// copy the encrypted block over to the target
		// the buffer is shifted by the block count (output + shift -. pointer to output[shift])
		memcpy(output + shift, _block.ciphertext, SOC_ECB_CIPHERTEXT_LENGTH);
	}

	return true;
}


/**
 * Check if the key that we need is set in the memory and if so, set it into the encryption block
 */
bool EncryptionHandler::_checkAndSetKey(uint8_t userLevel) {
	// check if the userLevel is set.
	if (userLevel == NOT_SET) {
		LOGe("User level is not set.");
		return false;
	}

	ConfigurationTypes keyConfigType;
	switch (userLevel) {
	case ADMIN:
		keyConfigType = CONFIG_KEY_AMIN;
		break;
	case MEMBER:
		keyConfigType = CONFIG_KEY_MEMBER;
		break;
	case GUEST:
		keyConfigType = CONFIG_KEY_GUEST;
		break;
	case SETUP: {
		if (_operationMode == OPERATION_MODE_SETUP && _setupKeyValid) {
			memcpy(_block.key, _setupKey, SOC_ECB_KEY_LENGTH);
			return true;
		}
		LOGe("Can't use this setup key");
		return false;
	}
	default:
		LOGe("Invalid user level");
//		LOGe("Provided userLevel does not exist (level is not admin, member or guest)");
		return false;
	}

	// get the key from the storage
	Settings::getInstance().get(keyConfigType, _block.key);
	return true;
}

/**
 * This method will fill the buffer with 3 random bytes. These are included in the message
 */
void EncryptionHandler::_generateNonceInTarget(uint8_t* target) {
#ifdef TESTING_ENCRYPTION
	memset(target, 128 ,PACKET_NONCE_LENGTH);
#else
	RNG::fillBuffer(target, PACKET_NONCE_LENGTH);
#endif
}

/**
 * This method will fill the buffer with 5 random bytes. This is done on connect and is only valid once.
 */
void EncryptionHandler::_generateSessionNonce() {
#ifdef TESTING_ENCRYPTION
	memset(_sessionNonce, 64 ,SESSION_NONCE_LENGTH);
#else
	RNG::fillBuffer(_sessionNonce, SESSION_NONCE_LENGTH);
#endif
	EventDispatcher::getInstance().dispatch(EVT_SESSION_NONCE_SET, _sessionNonce, SESSION_NONCE_LENGTH);
}

/**
 * The Nonce of the CTR method is made up of 8 random bytes:
 * 	- 3 from the packet nonce
 * 	- 5 from the session nonce
 * We use the term IV (initialization vector) for this because we have enough Nonce terms floating around.
 */
void EncryptionHandler::_createIV(uint8_t* IV, uint8_t* nonce, EncryptionType encryptionType) {
	// Set the nonce to zero to ensure it is consistent.
	// Since we only have 1 uint8_t as counter doing this once is enough.
	memset(IV, 0x00, SOC_ECB_CLEARTEXT_LENGTH);
	// half the text is nonce, the other half is a counter
	memcpy(IV, nonce, PACKET_NONCE_LENGTH);

	if (encryptionType == CTR_CAFEBABE) {
		// in case we do not use the session nonce, we use the cafebabe as a validation
		memcpy(IV + PACKET_NONCE_LENGTH, _defaultValidationKey.a, SESSION_NONCE_LENGTH);
	}
	else {
		// copy the session nonce over into the target
		memcpy(IV + PACKET_NONCE_LENGTH, _sessionNonce, SESSION_NONCE_LENGTH);
	}
}


/**
 * Verify if the block length is correct
 */
bool EncryptionHandler::_validateBlockLength(uint16_t length) {
	if (length % SOC_ECB_CLEARTEXT_LENGTH != 0 || length == 0) {
		return false;
	}
	return true;
}



bool EncryptionHandler::allowAccess(EncryptionAccessLevel minimum, EncryptionAccessLevel provided) {
	if (minimum != ENCRYPTION_DISABLED) {
		if (_operationMode == OPERATION_MODE_SETUP && provided == SETUP && _setupKeyValid) {
			return true;
		}

		// 0 is the highest possible value: ADMIN. If the provided is larger than the minumum, it is not allowed.
		if (provided > minimum) {
//			LOGi("Insufficient accessLevel provided to use the characteristic.");
			LOGi("Insufficient access.");
			return false;
		}
	}

	return true;
}

uint8_t* EncryptionHandler::generateNewSetupKey() {
	if (_operationMode == OPERATION_MODE_SETUP) {
		RNG::fillBuffer(_setupKey, SOC_ECB_KEY_LENGTH);
		_setupKeyValid = true;
	}
	else {
		_setupKeyValid = false;
	}

	return &(_setupKey[0]);
}

void EncryptionHandler::invalidateSetupKey() {
	_setupKeyValid = false;
}
