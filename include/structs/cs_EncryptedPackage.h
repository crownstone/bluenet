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

enum EncryptionUserLevels {
	ADMIN,
	USER,
	GUEST
};

struct __attribute__((__packed__)) encrypted_package_t {
	uint8_t nonce[8];
	uint8_t userLevel;
	uint16_t bufferLength = 0;
	uint8_t buffer[1];
};

class EncryptedPackage {
private:
	EncryptedPackage();
	~EncryptedPackage();

	uint8_t* _nonce;
	uint8_t* _cipherText;
	uint8_t* _clearText;
	uint8_t* _aesKey;

	nrf_ecb_hal_data_t* _block;

public:
	static EncryptedPackage& getInstance() {
		static EncryptedPackage instance;
		return instance;
	}

	void init();

	void encrypt(uint8_t* data, uint16_t dataLength, encrypted_package_t& target);
	void encryptAdvertisement();
	void decrypt(encrypted_package_t* data, uint8_t* target);
	void generateNonce(encrypted_package_t& targetPackage);



	ConfigurationTypes _getConfigType(uint8_t userLevel);
	bool _validateBlockLength(uint16_t length);
	void _generateNonce(uint8_t* target);
};

