/*
 * cs_EncryptedPackage.cpp
 *
 *  Created on: Jul 19, 2016
 *      Author: alex
 */



#include <structs/cs_EncryptedPackage.h>
#include <storage/cs_Settings.h>
#include <drivers/cs_RNG.h>



EncryptedPackage::~EncryptedPackage() {
	delete _nonce;
	delete _cipherText;
	delete _clearText;
	delete _aesKey;
}

void EncryptedPackage::init() {
	_nonce 		= new uint8_t[SOC_ECB_CLEARTEXT_LENGTH];  // 16 byte
	_cipherText = new uint8_t[SOC_ECB_CIPHERTEXT_LENGTH]; // 16 byte
	_clearText 	= new uint8_t[SOC_ECB_CLEARTEXT_LENGTH];  // 16 byte
	_aesKey 	= new uint8_t[SOC_ECB_KEY_LENGTH];
	_block 		= new nrf_ecb_hal_data_t __aligned(4);

	// set the nonce to zero to ensure it is consistent.
	memset(_nonce, 0x00, SOC_ECB_CLEARTEXT_LENGTH);
}

/**
 * This method does not use the AES CTR method. It encrypts the block with the GUEST key.
 */
void EncryptedPackage::encryptAdvertisement() {}

/**
 * This method encrypts the data and places the result into the target struct.
 * It is IMPORTANT that the target HAS a bufferlength that is dividable by 16 and not 0.
 * The buffer of the target NEEDS to be allocated for the bufferLength
 */
void EncryptedPackage::encrypt(uint8_t* data, uint16_t dataLength, encrypted_package_t& target) {

	// get the key from the storage
	ConfigurationTypes configKeyType = _getConfigType(target.userLevel);
	Settings::getInstance().get(configKeyType, _aesKey);


	// ensure the length of the encrypted buffer is a multiple of the block length
	if (_validateBlockLength(target.bufferLength)) {
		_generateNonce(target.nonce);
		memcpy(_nonce, target.nonce, 8);

		uint16_t ctr = 0;

		// gen cleartext
		memcpy(_block->key,		 _aesKey, 16);
		memcpy(_block->cleartext, _nonce,  16);

		// encrypts the cleartext and puts it in ciphertext
		sd_ecb_block_encrypt(_block);

	}
}

void EncryptedPackage::decrypt(encrypted_package_t* data, uint8_t* target) {

}

void EncryptedPackage::_generateNonce(uint8_t* target) {
	RNG::fillBuffer(target, 8);
}



ConfigurationTypes EncryptedPackage::_getConfigType(uint8_t userLevel) {
	switch (userLevel) {
	case ADMIN:
		return CONFIG_KEY_OWNER;
	case USER:
		return CONFIG_KEY_MEMBER;
	case GUEST:
		return CONFIG_KEY_GUEST;
	}
	return CONFIG_KEY_GUEST;
}

bool EncryptedPackage::_validateBlockLength(uint16_t length) {
	if (length % 16 != 0 || length == 0) {
		// throw or print error?
		return false;
	}
	return true;
}



/**
 * This method generates the encrypted blocks of length N from the Nonce, key and counter.
 *
 * 1) we prepare an encryption packet (nrf_ecb_hal_data_t) and load the key into it
 * 2) we create a combined 16bit cleartext from the Nonce and the counter
 * 		(8byte Nonce concat 8byte counter)
 * 3) we run the encryption for as long as we have to until the buffer from 1) is full
 */
void generateBlocks() {

	// prepare block for encryption
	nrf_ecb_hal_data_t ecb __aligned(4);
	uint8_t ctr = 0;
	uint8_t aesKey[SOC_ECB_KEY_LENGTH];
	uint8_t cleartext[SOC_ECB_KEY_LENGTH];

	// gen cleartext

	memset(ecb.ciphertext,0x00, 		sizeof(ecb.ciphertext)); // not needed
	memcpy(ecb.key,       aesKey, 		sizeof(ecb.key));
	memcpy(ecb.cleartext, cleartext,	sizeof(ecb.cleartext));

	// encrypts the cleartext and puts it in ciphertext
	sd_ecb_block_encrypt(&ecb);

}


//void decrypt() {
//	uint32_t data[...];
//	int length = sizeof(data);
//
//	/* decryption is just an xor using a key stream generated
//	* by AES encyption hardware.
//	*/
//	for (int w = 0; w < length / sizeof(uint32_t); w++)
//		data[w] ^= keyStream();
//}

/* keyStream: Return next word in output feedback sequence */
//static uint32_t keyStream(void)
//{
//    uint32_t key;
//
//    /* do we need more key data? */
//    if (keyStreamIdx == 0)
//    {
//        /* generate more key data */
//        sd_ecb_block_encrypt(&ecb);
//
//        /* cleartext for next round is ciphertext from this round */
//        memcpy(ecb.cleartext, ecb.ciphertext, sizeof(ecb.cleartext));
//    }
//
//    key = ((uint32_t*)ecb.ciphertext)[keyStreamIdx++];
//    if (keyStreamIdx == sizeof(ecb.ciphertext) / 4)
//        keyStreamIdx = 0; // will need more data next round
//
//    return key;
//}
