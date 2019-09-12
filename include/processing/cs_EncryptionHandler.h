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

#define PACKET_NONCE_LENGTH  	3
#define USER_LEVEL_LENGTH 		1
#define VALIDATION_NONCE_LENGTH 4
#define SESSION_NONCE_LENGTH 	5
#define DEFAULT_SESSION_KEY 	0xCAFEBABE
#define DEFAULT_SESSION_KEY_LENGTH 4

#define RC5_ROUNDS 12
#define RC5_NUM_SUBKEYS (2*(RC5_ROUNDS+1)) // t = 2(r+1) - the number of round subkeys required.
#define RC5_KEYLEN 16

enum EncryptionType {
	CTR,
	CTR_CAFEBABE,
	ECB_GUEST,
	ECB_GUEST_CAFEBABE
};

/** EncryptionHandler is reponsible for encrypting each BLE message.
 */
class EncryptionHandler : EventListener {
private:
	EncryptionHandler()  {}
	~EncryptionHandler() {}

	OperationMode _operationMode;
	uint8_t _sessionNonce[SESSION_NONCE_LENGTH];
	uint8_t _setupKey[SOC_ECB_KEY_LENGTH];
	bool _setupKeyValid = false;

	conv8_32 _defaultValidationKey;
	uint8_t _overhead = PACKET_NONCE_LENGTH + USER_LEVEL_LENGTH;
	uint16_t _rc5SubKeys[RC5_NUM_SUBKEYS]; // S[] - The round subkey words.

public:
	nrf_ecb_hal_data_t _block __attribute__ ((aligned (4)));
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
	 * Decrypt a single block in CTR mode.
	 */
	bool decryptBlockCTR(uint8_t* encryptedData, uint16_t encryptedDataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel accessLevel, uint8_t* nonce, uint8_t nonceLength);

	/**
	 * Initialized the key, using the key of given access level.
	 */
	bool RC5InitKey(EncryptionAccessLevel accessLevel);

	/**
	 * Decrypt data with RC5.
	 */
	bool RC5Decrypt(uint16_t* encryptedData, uint16_t encryptedDataLength, uint16_t* target, uint16_t targetLength);

	/**
	 * make sure we create a new nonce for each connection
	 */
	void handleEvent(event_t & event);

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

	bool allowedToWrite();




	void aggregateXor(uint8_t* dest, const uint8_t* const src_a, const uint8_t* const src_b,const size_t& len);

	void EncryptCtr(
		const uint8_t* const plainText,
		size_t plainTextLength, 
		uint8_t* cipherText,
		size_t cipherTextLength,
		nrf_ecb_hal_data_t& block);

	void DecryptCtr(
		uint8_t* plainText,
		size_t plainTextLength, 
		const uint8_t* const cipherText,
		size_t cipherTextLength,
		nrf_ecb_hal_data_t& block);

	bool TestCtrEncryption();
// TODO: undo this...
	uint32_t EncryptCtrSingleBlock(
		const uint8_t* const plainText, 
		size_t plainTextLength, 
		nrf_ecb_hal_data_t& block);
private:

	inline bool _encryptECB(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType);
	inline bool _prepareEncryptCTR(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength, EncryptionAccessLevel userLevel, EncryptionType encryptionType);
	inline bool _encryptCTR(uint8_t* validationNonce, uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength);
	inline bool _decryptCTR(uint8_t* validationNonce, uint8_t* input, uint16_t inputLength, uint8_t* target, uint16_t targetLength);
	bool _checkAndSetKey(uint8_t userLevel);
	bool _validateBlockLength(uint16_t length);
	bool _validateDecryption(uint8_t* buffer, uint8_t* validationNonce);
	void _generateSessionNonce();
	void _generateNonceInTarget(uint8_t* target);
	void _createIV(uint8_t* target, uint8_t* nonce, EncryptionType encryptionType);
	bool _RC5PrepareKey(uint8_t* key, uint8_t keyLength);
};

