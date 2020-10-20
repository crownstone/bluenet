/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_EncryptionHandler.h>
#include <storage/cs_State.h>
#include <encryption/cs_AES.h>
#include <encryption/cs_KeysAndAccess.h>

#include <drivers/cs_RTC.h>

//#define TESTING_ENCRYPTION
#define LOGEncryption LOGd

// Rotate functions for 16 bit integers
#define ROTL_16BIT(x, shift) ((x)<<(shift) | (x)>>(16-(shift)))
#define ROTR_16BIT(x, shift) ((x)>>(shift) | (x)<<(16-(shift)))

// Magic numbers for RC5 with 16 bit words.
#define RC5_16BIT_P 0xB7E1
#define RC5_16BIT_Q 0x9E37

void EncryptionHandler::init() {
	_defaultValidationKey.asInt = DEFAULT_VALIDATION_KEY;

	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	_operationMode = getOperationMode(mode);

	EventDispatcher::getInstance().addListener(this);
}

uint16_t EncryptionHandler::calculateEncryptionBufferLength(uint16_t inputLength, EncryptionType encryptionType) {
	if (encryptionType == CTR) {
		// add the validation padding length to the raw data length
		uint16_t requiredLength = inputLength + VALIDATION_KEY_LENGTH;

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
	uint16_t overhead = VALIDATION_KEY_LENGTH + PACKET_NONCE_LENGTH + USER_LEVEL_LENGTH;
	// catch case where the length can be smaller than the overhead and the int overflows.
	if (encryptedPacketLength <= overhead) {
		return 0;
	}
	return encryptedPacketLength - overhead;
}

void EncryptionHandler::handleEvent(event_t & event) {
	switch (event.type) {
	case CS_TYPE::EVT_BLE_CONNECT:
		if (State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED)) {
			_generateNewSetupKey();
			_generateSessionData();
		}
		break;
	default: {}
	}
}




void EncryptionHandler::closeConnectionAuthenticationFailure() {
	Stack::getInstance().disconnect();
}


bool EncryptionHandler::allowedToWrite() {
	return Stack::getInstance().isDisconnecting();
}

/**
 * Perform encryption:
 */
bool EncryptionHandler::encrypt(
		uint8_t* data, uint16_t dataLength,
		uint8_t* target, uint16_t targetLength,
		EncryptionAccessLevel userLevel, EncryptionType encryptionType)
{
	if (!KeysAndAccess::getInstance().getKey(userLevel, _key, sizeof(_key))) {
		return false;
	}

	switch (encryptionType) {
		case CTR:{
			uint32_t start = RTC::getCount();

			// Set the nonce.
			RNG::fillBuffer(_nonce.packetNonce, sizeof(_nonce.packetNonce));
			memcpy(_nonce.sessionNonce, _sessionData.sessionNonce, sizeof(_sessionData.sessionNonce));

			// Set the non encrypted header
			encryption_header_t* header = reinterpret_cast<encryption_header_t*>(target);
			if (targetLength < sizeof(*header)) {
				LOGw("target buffer too small for header");
				return false;
			}
			memcpy(header->packetNonce, _nonce.packetNonce, sizeof(_nonce.packetNonce));
			header->accessLevel = userLevel;

			// Encrypt the payload
			cs_buffer_size_t writtenSize;
			cs_ret_code_t retCode = AES::getInstance().encryptCtr(
					cs_data_t(_key, sizeof(_key)),
					cs_data_t((uint8_t*)&_nonce, sizeof(_nonce)),
					cs_data_t(_sessionData.validationKey, sizeof(_sessionData.validationKey)),
					cs_data_t(data, dataLength),
					cs_data_t(target + sizeof(*header), targetLength - sizeof(*header)),
					writtenSize);

			uint32_t end = RTC::getCount();
			LOGi("new: %u ticks", RTC::difference(end, start));
//			BLEutil::printArray(target, targetLength);


			start = RTC::getCount();
			_prepareEncryptCTR(data,dataLength,target,targetLength,userLevel,encryptionType);
			end = RTC::getCount();

			LOGi("old: %u ticks", RTC::difference(end, start));
//			BLEutil::printArray(target, targetLength);


			return retCode == ERR_SUCCESS;
		}
		case ECB_GUEST_CAFEBABE:{
			cs_buffer_size_t writtenSize;
			cs_ret_code_t retCode = AES::getInstance().encryptEcb(
					cs_data_t(_key, sizeof(_key)),
					cs_data_t(_defaultValidationKey.asBuf, sizeof(_defaultValidationKey.asBuf)),
					cs_data_t(data, dataLength),
					cs_data_t(target, targetLength),
					writtenSize);

			return retCode == ERR_SUCCESS;
		}
		case ECB_GUEST: {
			cs_buffer_size_t writtenSize;
			cs_ret_code_t retCode = AES::getInstance().encryptEcb(
					cs_data_t(_key, sizeof(_key)),
					cs_data_t(),
					cs_data_t(data, dataLength),
					cs_data_t(target, targetLength),
					writtenSize);

			return retCode == ERR_SUCCESS;
		}
		default:
			return false;
	}
}

bool EncryptionHandler::decrypt(uint8_t* encryptedDataPacket, uint16_t encryptedDataPacketLength, uint8_t* target,
		uint16_t targetLength, EncryptionAccessLevel& levelOfPackage, EncryptionType encryptionType) {

	switch (encryptionType) {
		case CTR: {
			uint32_t start = RTC::getCount();

			// Parse the non encrypted header.
			encryption_header_t* header = reinterpret_cast<encryption_header_t*>(encryptedDataPacket);
			if (encryptedDataPacketLength < sizeof(*header)) {
				LOGw("target buffer too small for header");
				return false;
			}

			switch (header->accessLevel) {
				case ADMIN:
					levelOfPackage = ADMIN;
					break;
				case MEMBER:
					levelOfPackage = MEMBER;
					break;
				case BASIC:
					levelOfPackage = BASIC;
					break;
				case SETUP:
					levelOfPackage = SETUP;
					break;
				default:
					levelOfPackage = NOT_SET;
					break;
			}
//			LOGi("header->accessLevel=%u levelOfPackage=%u", header->accessLevel, levelOfPackage);

			if (!KeysAndAccess::getInstance().getKey(levelOfPackage, _key, sizeof(_key))) {
				return false;
			}

			// Set the nonce.
			memcpy(_nonce.packetNonce, header->packetNonce, sizeof(_nonce.packetNonce));
			memcpy(_nonce.sessionNonce, _sessionData.sessionNonce, sizeof(_nonce.sessionNonce));

			// Decrypt the payload.
			encryption_header_encrypted_t encryptedHeader;
			cs_ret_code_t retCode = AES::getInstance().decryptCtr(
					cs_data_t(_key, sizeof(_key)),
					cs_data_t((uint8_t*)&_nonce, sizeof(_nonce)),
					cs_data_t(encryptedDataPacket + sizeof(*header), encryptedDataPacketLength - sizeof(*header)),
					cs_data_t((uint8_t*)&encryptedHeader, sizeof(encryptedHeader)),
					cs_data_t(target, targetLength));

			uint32_t end = RTC::getCount();
			LOGi("new: %u ticks", RTC::difference(end, start));
//			BLEutil::printArray((uint8_t*)&encryptedHeader, sizeof(encryptedHeader));
//			BLEutil::printArray(target, targetLength);


			start = RTC::getCount();
			_decrypt(encryptedDataPacket, encryptedDataPacketLength, target, targetLength, levelOfPackage, encryptionType);
			end = RTC::getCount();

			LOGi("old: %u ticks", RTC::difference(end, start));
//			BLEutil::printArray(target, targetLength);


			// Check validation key.
			if (memcmp(encryptedHeader.validationKey, _sessionData.validationKey, sizeof(_sessionData.validationKey)) != 0) {
				LOGEncryption("Validation mismatch [%u %u ..] vs [%u %u ..]", encryptedHeader.validationKey[0], encryptedHeader.validationKey[1], _sessionData.validationKey[0], _sessionData.validationKey[1]);
				return false;
			}

			return retCode == ERR_SUCCESS;
		}
		default:
			LOGe("Wrong type: %u", encryptionType);
			return false;
	}
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
		memcpy(IV + PACKET_NONCE_LENGTH, _defaultValidationKey.asBuf, SESSION_NONCE_LENGTH);
	}
	else {
		// copy the session nonce over into the target
		memcpy(IV + PACKET_NONCE_LENGTH, _sessionData.sessionNonce, SESSION_NONCE_LENGTH);
	}
}










/**
 * This is where the magic happens. There are a few things that have to be done before this method is called:
 * 1) The correct key has to be set in the _block.key
 * 2) The correct Nonce has to be set in the _block.clearText
 *
 * The outputLength should be larger than 0 and a multiple of 16 or this will return false.
 * It is possible that the input is smaller than the output, in this case it will be zero padded.
 *
 * The validation key is automatically added to the input.
 */
bool EncryptionHandler::_encryptCTR(uint8_t* validationKey, uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength) {
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
			// if we are at the first block, we will add the validation key to the first 4 bytes
			if (shift == 0 && i < VALIDATION_KEY_LENGTH) {
				_block.ciphertext[i] ^= *(validationKey + i);
			} else {
				inputReadIndex = i + shift - VALIDATION_KEY_LENGTH;

				if (inputReadIndex < inputLength)
					_block.ciphertext[i] ^= input[inputReadIndex];
				else
					_block.ciphertext[i] ^= 0; // zero padding the data to fit in the block.
			}
		}


		// copy the encrypted block over to the target
		// the buffer is shifted by the block count (output + shift -. pointer to output[shift])
		memcpy(output + shift, _block.ciphertext, SOC_ECB_CIPHERTEXT_LENGTH);
	}

	return true;
}















/**
 */
bool EncryptionHandler::_prepareEncryptCTR(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength,
		EncryptionAccessLevel userLevel, EncryptionType encryptionType) {
//	LOGEncryption("Encrypt CTR level=%u", userLevel);

	// check if the userLevel has been set
	if (_checkAndSetKey(userLevel) == false) {
		LOGe("User level not set");
		return false;
	}

	uint16_t targetNetLength = targetLength - _overhead;

	// check if the input would still fit if the validation is added.
	if (dataLength + VALIDATION_KEY_LENGTH > targetNetLength) {
		LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		return false;
	}

	// generate a Nonce for this session and write it to the first 3 bytes of the target.
	_generateNonceInTarget(target);

//	 write the userLevel to the target
	target[PACKET_NONCE_LENGTH] = uint8_t(userLevel);

	// create the IV
	_createIV(_block.cleartext, target, encryptionType);

	uint8_t* validationKey;
	if (encryptionType == CTR_CAFEBABE) {
		// in case we need a checksum but not a session, we use 0xcafebabe
		validationKey = _defaultValidationKey.asBuf;
	}
	else {
		validationKey = _sessionData.validationKey;
	}

	if (_encryptCTR(validationKey, data, dataLength, target+_overhead, targetNetLength) == false) {
		LOGe("Error while encrypting");
		return false;
	}

	return true;
}



bool EncryptionHandler::_decrypt(uint8_t* encryptedDataPacket, uint16_t encryptedDataPacketLength, uint8_t* target,
		uint16_t targetLength, EncryptionAccessLevel& levelOfPackage, EncryptionType encryptionType) {
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
			levelOfPackage = BASIC; break;
		case 100:
			levelOfPackage = SETUP; break;
		default:
			levelOfPackage = NOT_SET; break;
	}

	// the actual encrypted part is after the overhead
	uint16_t sourceNetLength = encryptedDataPacketLength - _overhead;

	// setup the IV
	_createIV(_block.cleartext, encryptedDataPacket, encryptionType);

	uint8_t* validationKey;
	if (encryptionType == CTR_CAFEBABE) {
		validationKey = _defaultValidationKey.asBuf;
	}
	else {
		validationKey = _sessionData.validationKey;
	}

	if (_decryptCTR(validationKey, encryptedDataPacket + _overhead, sourceNetLength, target, targetLength) == false) {
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
bool EncryptionHandler::_decryptCTR(uint8_t* validationKey, uint8_t* input, uint16_t inputLength, uint8_t* target, uint16_t targetLength) {
//	LOGEncryption("Decrypt CTR");
	if (_validateBlockLength(inputLength) == false) {
		LOGe(STR_ERR_MULTIPLE_OF_16);
//		LOGe("Length of input block does not match the expected length (N*16 Bytes).");
		return false;
	}
	uint32_t err_code;

	// amount of blocks to loop over
	uint8_t blockOverflow = (targetLength + VALIDATION_KEY_LENGTH) %  SOC_ECB_CIPHERTEXT_LENGTH;
	uint16_t blockCount = (targetLength + VALIDATION_KEY_LENGTH) / SOC_ECB_CIPHERTEXT_LENGTH;
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
//		LOGi("key:");
//		BLEutil::printArray(_block.key, sizeof(_block.key));
//		LOGi("cleartext:");
//		BLEutil::printArray(_block.cleartext, sizeof(_block.cleartext));
		err_code = sd_ecb_block_encrypt(&_block);
		APP_ERROR_CHECK(err_code);
//		memcpy(_block.ciphertext, _block.cleartext, sizeof(_block.ciphertext));
//		LOGi("cipher:");
//		BLEutil::printArray(_block.ciphertext, sizeof(_block.ciphertext));



		// XOR the ciphertext with the data to finish encrypting the block.
		for (uint8_t i = 0; i < SOC_ECB_CIPHERTEXT_LENGTH; i++) {
			inputReadIndex = i + counter * SOC_ECB_CIPHERTEXT_LENGTH;
			if (inputReadIndex < inputLength)
				_block.ciphertext[i] ^= input[inputReadIndex];
			else {
				LOGe("Length mismatch %d, counter: %d, i: %d", inputReadIndex, counter, i);
				return false;
			}
		}


		// check the validation key
		if (counter == 0) {
			if (_validateDecryption(_block.ciphertext, validationKey) == false)
				return false;

			// the first iteration we can only write 12 relevant bytes to the target because the first 4 are the nonce.
			maxIterationWriteAmount = SOC_ECB_CIPHERTEXT_LENGTH - VALIDATION_KEY_LENGTH;
			if (targetLengthLeftToWrite < maxIterationWriteAmount)
				maxIterationWriteAmount = targetLengthLeftToWrite;

			// we offset the pointer of the cipher text by the length of the validation length
			memcpy(target, _block.ciphertext + VALIDATION_KEY_LENGTH, maxIterationWriteAmount);

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
 * Checks if the first 4 bytes of the decrypted buffer match the validation key.
 */
bool EncryptionHandler::_validateDecryption(uint8_t* buffer, uint8_t* validationKey) {
	for (uint8_t i = 0; i < VALIDATION_KEY_LENGTH; i++) {
		if (buffer[i] != *(validationKey + i)) {
			LOGe("Nonce mismatch [%u %u ..] vs [%u %u ..]", buffer[0], buffer[1], *(validationKey + 0), *(validationKey + 1));
			return false;
		}
	}
	return true;
}

bool EncryptionHandler::decryptBlockCTR(uint8_t* encryptedData, uint16_t encryptedDataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel accessLevel, uint8_t* nonce, uint8_t nonceLength) {
	if (encryptedDataLength < SOC_ECB_CIPHERTEXT_LENGTH || targetLength < SOC_ECB_CIPHERTEXT_LENGTH) {
		LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		return false;
	}

	// Sets the key in _block.
	if (!_checkAndSetKey(accessLevel)) {
		return false;
	}
//	LOGi("key:");
//	BLEutil::printArray(_block.key, sizeof(_block.key));

	// Set the IV in the cleartext. IV is simply nonce with zero padding, since the counter is 0.
	memset(_block.cleartext, 0x00, SOC_ECB_CLEARTEXT_LENGTH);
	memcpy(_block.cleartext, nonce, nonceLength);
//	LOGd("cleartext:");
//	BLEutil::printArray(_block.cleartext, 16);

	// encrypts the cleartext and puts it in ciphertext
	uint32_t errCode = sd_ecb_block_encrypt(&_block);
	APP_ERROR_CHECK(errCode);

	// XOR the ciphertext with the encrypted data to finish decrypting the block.
	for (uint8_t i = 0; i < SOC_ECB_CIPHERTEXT_LENGTH; i++) {
		target[i] = _block.ciphertext[i] ^ encryptedData[i];
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

	CS_TYPE keyConfigType;
	switch (userLevel) {
	case ADMIN:
		keyConfigType = CS_TYPE::CONFIG_KEY_ADMIN;
		break;
	case MEMBER:
		keyConfigType = CS_TYPE::CONFIG_KEY_MEMBER;
		break;
	case BASIC:
		keyConfigType = CS_TYPE::CONFIG_KEY_BASIC;
		break;
	case SERVICE_DATA:
		keyConfigType = CS_TYPE::CONFIG_KEY_SERVICE_DATA;
		break;
	case LOCALIZATION:
		keyConfigType = CS_TYPE::CONFIG_KEY_LOCALIZATION;
		break;
	case SETUP: {
		if (_operationMode == OperationMode::OPERATION_MODE_SETUP && _setupKeyValid) {
			memcpy(_block.key, _setupKey, SOC_ECB_KEY_LENGTH);
			return true;
		}
		// This error is generated once on boot
		LOGe("Can't use this setup key (yet)");
		return false;
	}
	default:
		LOGe("Invalid user level");
		return false;
	}

	// get the key from the storage
	static int log_once = 1;
	if (log_once) {
		LOGd("Get key - level %i - from storage", userLevel);
		log_once = 0;
	}
	State::getInstance().get(keyConfigType, _block.key, SOC_ECB_CIPHERTEXT_LENGTH);
	return true;
}

/**
 * This method will fill the buffer with 3 random bytes. These are included in the message
 */
void EncryptionHandler::_generateNonceInTarget(uint8_t* target) {
#ifdef TESTING_ENCRYPTION
	memset(target, 128, PACKET_NONCE_LENGTH);
#else
	RNG::fillBuffer(target, PACKET_NONCE_LENGTH);
//	memcpy(target, _nonce.packetNonce, sizeof(_nonce.packetNonce));
#endif
}

/**
 * This method will fill the buffer with 5 random bytes. This is done on connect and is only valid once.
 */
void EncryptionHandler::_generateSessionData() {
	_sessionData.protocol = CS_CONNECTION_PROTOCOL_VERSION;
#ifdef TESTING_ENCRYPTION
	memset(_sessionData.sessionNonce, 64, sizeof(_sessionData.sessionNonce));
	memset(_sessionData.validationKey, 65, sizeof(_sessionData.validationKey));
#else
	RNG::fillBuffer(_sessionData.sessionNonce, sizeof(_sessionData.sessionNonce));
	RNG::fillBuffer(_sessionData.validationKey, sizeof(_sessionData.validationKey));
#endif
	event_t event(CS_TYPE::EVT_SESSION_DATA_SET, &_sessionData, sizeof(_sessionData));
	EventDispatcher::getInstance().dispatch(event);
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
	// always allow access when encryption is disabled.
	if (State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED) == false) {
		return true;
	}

	if (minimum == NOT_SET || minimum == NO_ONE) {
		return false;
	}

	if (minimum == ENCRYPTION_DISABLED) {
		return true;
	}

	if (_operationMode == OperationMode::OPERATION_MODE_SETUP && provided == SETUP && _setupKeyValid) {
		return true;
	}

	if (minimum == SETUP && (provided != SETUP || _operationMode != OperationMode::OPERATION_MODE_SETUP || !_setupKeyValid)) {
		LOGw("Setup mode required");
		return false;
	}

	// 0 is the highest possible value: ADMIN. If the provided is larger than the minimum, it is not allowed.
	if (provided > minimum) {
		LOGw("Insufficient access");
		return false;
	}

	return true;
}

uint8_t* EncryptionHandler::getSetupKey() {
	if (!_setupKeyValid) {
		return NULL;
	}
	return _setupKey;
}

void EncryptionHandler::_generateNewSetupKey() {
	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
		RNG::fillBuffer(_setupKey, SOC_ECB_KEY_LENGTH);
		_setupKeyValid = true;
	}
	else {
		_setupKeyValid = false;
	}
}

void EncryptionHandler::invalidateSetupKey() {
	_setupKeyValid = false;
}
