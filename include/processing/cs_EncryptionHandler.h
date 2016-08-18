/*
 * Author: Alex de Mulder
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: July 19, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdlib>
#include <protocol/cs_ConfigTypes.h>
#include "nrf_soc.h"
#include <drivers/cs_RNG.h>
#include <events/cs_EventListener.h>

#define PACKET_NONCE_LENGTH  	3
#define USER_LEVEL_LENGTH 		1
#define VALIDATION_NONCE_LENGTH 4
#define SESSION_NONCE_LENGTH 	5
#define DEFAULT_SESSION_KEY 	0xcafebabe
#define DEFAULT_SESSION_KEY_LENGTH 4

enum EncryptionAccessLevel {
	ADMIN               = 0,
	MEMBER              = 1,
	GUEST               = 2,
	SETUP               = 100,
	NOT_SET             = 201,
	ENCRYPTION_DISABLED = 255
};

enum EncryptionType {
	CTR,
	CTR_CAFEBABE,
	ECB_GUEST,
	ECB_GUEST_CAFEBABE
};

class EncryptionHandler : EventListener {
private:
	EncryptionHandler()  {}
	~EncryptionHandler() {}

	uint8_t _operationMode;
	uint8_t _sessionNonce[SESSION_NONCE_LENGTH];
	nrf_ecb_hal_data_t _block __attribute__ ((aligned (4)));
	uint8_t _setupKey[SOC_ECB_KEY_LENGTH];
	bool _setupKeyValid = false;

	conv8_32 _defaultValidationKey;
	uint8_t _overhead = PACKET_NONCE_LENGTH + USER_LEVEL_LENGTH;

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
	 * The mesh only uses cafebabe as a validation and the admin key to encrypt. Other than that, its the same.
	 */
	bool encryptMesh(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength);

	/**
	 * This method decrypts the package and places the decrypted blocks, in full, in the buffer. After this
	 * the result will be validated and the target will be populated.
	 */
	bool decrypt(uint8_t* encryptedDataPacket, uint16_t encryptedDataPacketLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel& userLevelInPackage, EncryptionType encryptionType = CTR);

	/**
	 * make sure we create a new nonce for each connection
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	/**
	 * Allow characteristics to get the sessionNonce. Length of the sessionNonce is 5 bytes.
	 */
	uint8_t* getSessionNonce();

	/**
	 * Break the connection if there is an error in the encryption or decryption
	 */
	void closeConnectionAuthenticationFailure();

	/**
	 * Verify if the access level provided is sufficient
	 */
	bool allowAccess(EncryptionAccessLevel minimum, EncryptionAccessLevel provided);

	/**
	 * Determine the required size of the encryption buffer based on what you want to encrypt.
	 */
	static uint16_t calculateEncryptionBufferLength(uint16_t inputLength, EncryptionType encryptionType = CTR);

	/**
	 * Determine the required size of the decryption buffer based on how long the encrypted packet is.
	 * The encrypted packet is the total size of what was written to the characteristic.
	 */
	static uint16_t calculateDecryptionBufferLength(uint16_t encryptedPacketLength);

	/**
	 * Generate a 16 byte key to be used during the setup phase.
	 */
	uint8_t* generateNewSetupKey();

	/**
	 * After the setup phase, invalidate the key. Is usually called on disconnect.
	 */
	void invalidateSetupKey();

private:

	inline bool _encryptECB(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType);
	inline bool _prepareEncryptCTR(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType);
	inline bool _encryptCTR(uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength, EncryptionType encryptionType);
	inline bool _decryptCTR(uint8_t* input, uint16_t inputLength, uint8_t* target, uint16_t targetLength, EncryptionType encryptionType);
	bool _checkAndSetKey(uint8_t userLevel);
	bool _validateBlockLength(uint16_t length);
	bool _validateDecryption(uint8_t* buffer, EncryptionType encryptionType);
	void _generateSessionNonce();
	void _generateNonceInTarget(uint8_t* target);
	void _createIV(uint8_t* target, uint8_t* nonce, EncryptionType encryptionType);

};

