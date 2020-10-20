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

//#define TESTING_ENCRYPTION
#define LOGEncryption LOGnone

void EncryptionHandler::init() {
	_defaultValidationKey.asInt = DEFAULT_VALIDATION_KEY;

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
//			_generateNewSetupKey();
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

