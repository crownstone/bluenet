/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 21, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <common/cs_Types.h>
#include <drivers/cs_RNG.h>
#include <encryption/cs_AES.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_KeysAndAccess.h>
#include <events/cs_Event.h>
#include <logging/cs_Logger.h>
#include <util/cs_Utils.h>

#define LOGConnectionEncryption LOGvv
#define LogLevelConnectionEncryption SERIAL_VERY_VERBOSE

ConnectionEncryption::ConnectionEncryption() {
	_sessionData.protocol = CS_CONNECTION_PROTOCOL_VERSION;
}

void ConnectionEncryption::init() {
	listen();
}

cs_ret_code_t ConnectionEncryption::encrypt(cs_data_t input, cs_data_t output, EncryptionAccessLevel accessLevel, ConnectionEncryptionType encryptionType) {
	uint8_t key[ENCRYPTION_KEY_LENGTH];
	if (!KeysAndAccess::getInstance().getKey(accessLevel, key, sizeof(key))) {
		return ERR_NOT_FOUND;
	}

	switch (encryptionType) {
		case ConnectionEncryptionType::CTR: {
			// Generate the packet nonce.
			RNG::fillBuffer(_nonce.packetNonce, sizeof(_nonce.packetNonce));

			// Set the non encrypted header
			encryption_header_t* header = reinterpret_cast<encryption_header_t*>(output.data);
			if (output.len < sizeof(*header)) {
				LOGw("target buffer too small for header");
				return ERR_BUFFER_TOO_SMALL;
			}
			memcpy(header->packetNonce, _nonce.packetNonce, sizeof(_nonce.packetNonce));
			header->accessLevel = accessLevel;

			// Encrypt the payload
			cs_buffer_size_t writtenSize;
			cs_ret_code_t retCode = AES::getInstance().encryptCtr(
					cs_data_t(key, sizeof(key)),
					cs_data_t((uint8_t*)&_nonce, sizeof(_nonce)),
					cs_data_t(_sessionData.validationKey, sizeof(_sessionData.validationKey)),
					input,
					cs_data_t(output.data + sizeof(*header), output.len - sizeof(*header)),
					writtenSize);

			return retCode;
		}
		case ConnectionEncryptionType::ECB: {
			conv8_32 validationKey;
			validationKey.asInt = DEFAULT_VALIDATION_KEY;
			cs_buffer_size_t writtenSize;
			cs_ret_code_t retCode = AES::getInstance().encryptEcb(
					cs_data_t(key, sizeof(key)),
					cs_data_t(validationKey.asBuf, sizeof(validationKey.asBuf)),
					input,
					output,
					writtenSize);

			return retCode;
		}
		default:
			return ERR_NOT_IMPLEMENTED;
	}
}


cs_ret_code_t ConnectionEncryption::decrypt(cs_data_t input, cs_data_t output, EncryptionAccessLevel& accessLevel, ConnectionEncryptionType encryptionType) {
	switch (encryptionType) {
		case ConnectionEncryptionType::CTR: {
			// Parse the non encrypted header.
			encryption_header_t* header = reinterpret_cast<encryption_header_t*>(input.data);
			if (input.len < sizeof(*header)) {
				LOGw("target buffer too small for header");
				return ERR_BUFFER_TOO_SMALL;
			}

			switch (header->accessLevel) {
				case ADMIN:
					accessLevel = ADMIN;
					break;
				case MEMBER:
					accessLevel = MEMBER;
					break;
				case BASIC:
					accessLevel = BASIC;
					break;
				case SETUP:
					accessLevel = SETUP;
					break;
				default:
					accessLevel = NOT_SET;
					break;
			}

			uint8_t key[ENCRYPTION_KEY_LENGTH];
			if (!KeysAndAccess::getInstance().getKey(accessLevel, key, sizeof(key))) {
				return ERR_NOT_FOUND;
			}

			// Set the nonce.
			memcpy(_nonce.packetNonce, header->packetNonce, sizeof(_nonce.packetNonce));
			memcpy(_nonce.sessionNonce, _sessionData.sessionNonce, sizeof(_nonce.sessionNonce));

			// Decrypt the payload.
			encryption_header_encrypted_t encryptedHeader;
			cs_buffer_size_t decryptedPayloadSize;
			cs_ret_code_t retCode = AES::getInstance().decryptCtr(
					cs_data_t(key, sizeof(key)),
					cs_data_t((uint8_t*)&_nonce, sizeof(_nonce)),
					cs_data_t(input.data + sizeof(*header), input.len - sizeof(*header)),
					cs_data_t((uint8_t*)&encryptedHeader, sizeof(encryptedHeader)),
					output,
					decryptedPayloadSize);

			// Check validation key.
			if (memcmp(encryptedHeader.validationKey, _sessionData.validationKey, sizeof(_sessionData.validationKey)) != 0) {
				LOGConnectionEncryption("Validation mismatch [%u %u ..] vs [%u %u ..]", encryptedHeader.validationKey[0], encryptedHeader.validationKey[1], _sessionData.validationKey[0], _sessionData.validationKey[1]);
				return ERR_NO_ACCESS;
			}

			return retCode;
		}
		default:
			LOGe("Wrong type: %u", encryptionType);
			return ERR_NOT_IMPLEMENTED;
	}
}




void ConnectionEncryption::generateSessionData() {
	RNG::fillBuffer(_sessionData.sessionNonce, sizeof(_sessionData.sessionNonce));
	RNG::fillBuffer(_sessionData.validationKey, sizeof(_sessionData.validationKey));
	memcpy(_nonce.sessionNonce, _sessionData.sessionNonce, sizeof(_sessionData.sessionNonce));
	_log(LogLevelConnectionEncryption, false, "Set session data:");
	_logArray(LogLevelConnectionEncryption, true, reinterpret_cast<uint8_t*>(&_sessionData), sizeof(_sessionData));
}

cs_ret_code_t ConnectionEncryption::setSessionData(session_data_t& sessionData) {
	if (sessionData.protocol != _sessionData.protocol) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}
//	memcpy(_sessionData.sessionNonce, sessionData.sessionNonce, sizeof(sessionData.sessionNonce));
//	memcpy(_sessionData.validationKey, sessionData.validationKey, sizeof(sessionData.validationKey));
	_sessionData = sessionData;
	memcpy(_nonce.sessionNonce, sessionData.sessionNonce, sizeof(sessionData.sessionNonce));
	_log(LogLevelConnectionEncryption, false, "Set session data:");
	_logArray(LogLevelConnectionEncryption, true, reinterpret_cast<uint8_t*>(&_sessionData), sizeof(_sessionData));
	return ERR_SUCCESS;
}


cs_buffer_size_t ConnectionEncryption::getEncryptedBufferSize(cs_buffer_size_t plaintextBufferSize, ConnectionEncryptionType encryptionType) {
	switch (encryptionType) {
		case ConnectionEncryptionType::CTR: {
			cs_buffer_size_t encryptedSize = plaintextBufferSize + sizeof(encryption_header_encrypted_t);
			return sizeof(encryption_header_t) + CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(encryptedSize, AES_BLOCK_SIZE);
		}
		case ConnectionEncryptionType::ECB:
		default:
			return CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(plaintextBufferSize, AES_BLOCK_SIZE);
	}
}


cs_buffer_size_t ConnectionEncryption::getPlaintextBufferSize(cs_buffer_size_t encryptedBufferSize, ConnectionEncryptionType encryptionType) {
	switch (encryptionType) {
		case ConnectionEncryptionType::CTR: {
			// Make sure we don't try to return a negative value.
			if (encryptedBufferSize < sizeof(encryption_header_t)) {
				return 0;
			}
			return encryptedBufferSize - sizeof(encryption_header_t);
		}
		case ConnectionEncryptionType::ECB:
		default:
			return encryptedBufferSize;
	}
}

bool ConnectionEncryption::allowedToWrite() {
	return Stack::getInstance().isDisconnecting();
}

void ConnectionEncryption::disconnect() {
	Stack::getInstance().disconnect();
}

void ConnectionEncryption::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_BLE_CONNECT: {
			// Both a new setup key and session data have to be generated.
			KeysAndAccess::getInstance().generateSetupKey();
			generateSessionData();

			// Send out an event that the session data (and setup key) is generated.
			// Let the compiler check the data type.
			TYPIFY(EVT_SESSION_DATA_SET)* eventData = &_sessionData;
			event_t event(CS_TYPE::EVT_SESSION_DATA_SET, eventData, sizeof(*eventData));
			event.dispatch();

			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED:
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			// End of connection session.
			KeysAndAccess::getInstance().invalidateSetupKey();
			break;
		}
		default:
			break;
	}
}

