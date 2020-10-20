/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: July 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <cstdlib>
#include <drivers/cs_RNG.h>
#include <events/cs_EventListener.h>
#include <storage/cs_State.h>

#define USER_LEVEL_LENGTH       1
#define DEFAULT_VALIDATION_KEY  0xCAFEBABE

enum EncryptionType {
	CTR,
	CTR_CAFEBABE,
	ECB_GUEST,
	ECB_GUEST_CAFEBABE
};

/** EncryptionHandler is reponsible for encrypting each BLE message.
 */
class EncryptionHandler : EventListener {
public:
	static EncryptionHandler& getInstance() {
		static EncryptionHandler instance;
		return instance;
	}

	/**
	 * Allocate memory for all required fields
	 */
	void init();

	/**
	 * This method encrypts the data and places the result into the target struct.
	 * It is IMPORTANT that the target HAS a bufferLength that is dividable by 16 and not 0.
	 * The buffer of the target NEEDS to be allocated for the bufferLength.
	 *
	 * data can be any number of bytes.
	 */
	bool encrypt(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType = CTR);

	/**
	 * This method decrypts the package and places the decrypted blocks, in full, in the buffer. After this
	 * the result will be validated and the target will be populated.
	 */
	bool decrypt(uint8_t* encryptedDataPacket, uint16_t encryptedDataPacketLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel& userLevelInPackage, EncryptionType encryptionType = CTR);

	/**
	 * make sure we create a new nonce for each connection
	 */
	void handleEvent(event_t & event);

	/**
	 * Break the connection if there is an error in the encryption or decryption
	 */
	void closeConnectionAuthenticationFailure();

	/**
	 * Determine the required size of the encryption buffer based on what you want to encrypt.
	 */
	static uint16_t calculateEncryptionBufferLength(uint16_t inputLength, EncryptionType encryptionType = CTR);

	/**
	 * Determine the required size of the decryption buffer based on how long the encrypted packet is.
	 * The encrypted packet is the total size of what was written to the characteristic.
	 */
	static uint16_t calculateDecryptionBufferLength(uint16_t encryptedPacketLength);

	bool allowedToWrite();
private:
	EncryptionHandler()  {}
	~EncryptionHandler() {}

	session_data_t _sessionData;

	conv8_32 _defaultValidationKey;

	uint8_t _key[ENCRYPTION_KEY_LENGTH];
	encryption_nonce_t _nonce;
	void _generateSessionData();
};

