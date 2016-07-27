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
#include <events/cs_EventListener.h>

#define PACKET_NONCE_LENGTH  3
#define SESSION_NONCE_LENGTH  5

enum EncryptionUserLevels {
	ADMIN 	= 0,
	USER 	= 1,
	GUEST 	= 2,
	NOT_SET = 255
};

struct __attribute__((__packed__)) encrypted_package_t {
	uint8_t nonce[3];
	uint8_t userLevel = NOT_SET; // has to be overwritten by the user supplying it.
	uint16_t bufferLength = 0;
	uint8_t buffer[1];
};

class EncryptedPackage : EventListener {
private:
	EncryptedPackage();
	~EncryptedPackage();

	uint8_t* _sessionNonce;
	nrf_ecb_hal_data_t* _block;
	bool _initialized = false;

public:
	static EncryptedPackage& getInstance() {
		static EncryptedPackage instance;
		return instance;
	}

	void init();

	void encrypt(uint8_t* data, uint16_t dataLength, encrypted_package_t& target);
	void encryptAdvertisement(uint8_t* data, uint8_t dataLength, uint8_t* target, uint8_t targetLength);
	void decrypt(encrypted_package_t& data, uint8_t* target, uint16_t targetLength);
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);
	uint8_t* getSessionNonce();

private:
	bool _performAES128CTR(uint8_t* input, uint16_t inputLength, uint8_t* output, uint16_t outputLength);
	bool _checkAndSetKey(uint8_t userLevel);
	bool _validateBlockLength(uint16_t length);
	void _generateSessionNonce();
	void _generateNewNonce(uint8_t* target);
	void _createIV(uint8_t* target, uint8_t* nonce);

};

