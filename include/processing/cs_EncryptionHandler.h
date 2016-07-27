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


enum EncryptionUserLevel {
	ADMIN 	= 0,
	USER 	= 1,
	GUEST 	= 2,
	NOT_SET = 255
};


class EncryptionHandler : EventListener {
private:
	EncryptionHandler()  {}
	~EncryptionHandler() {}

	uint8_t _sessionNonce[SESSION_NONCE_LENGTH];
	nrf_ecb_hal_data_t _block __attribute__ ((aligned (4)));

	conv8_32 _defaultValidationKey;
	uint8_t _overhead = PACKET_NONCE_LENGTH + USER_LEVEL_LENGTH;

public:
	static EncryptionHandler& getInstance() {
		static EncryptionHandler instance;
		return instance;
	}

	void init();

	bool encrypt(uint8_t* data, uint16_t dataLength, uint8_t* target, uint16_t targetLength, EncryptionUserLevel userLevel, bool useSessionNonce = true);
	bool encryptAdvertisement(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength);
	bool encryptMesh(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength);
	bool decrypt(uint8_t* encryptedDataPacket, uint16_t encryptedDataPacketLength, uint8_t* target, uint16_t targetLength,  bool useSessionNonce = true);
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);
	uint8_t* getSessionNonce();

private:
	bool _encryptCTR(uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength, bool useSessionNonce = true);
	bool _decryptCTR(uint8_t* input, uint16_t inputLength, uint8_t* target, uint16_t targetLength, bool useSessionNonce = true);
	bool _checkAndSetKey(uint8_t userLevel);
	bool _validateBlockLength(uint16_t length);
	bool _validateDecryption(uint8_t* buffer, bool useSessionNonce);
	void _generateSessionNonce();
	void _generateNonceInTarget(uint8_t* target);
	void _createIV(uint8_t* target, uint8_t* nonce, bool useSessionNonce);

};

