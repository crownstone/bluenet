/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_FirmwareReader.h>
#include <dfu/cs_FirmwareSections.h>

#include <logging/cs_Logger.h>

//#include <nrf_assert.h>
//#include <components/libraries/crypto/nrf_crypto.h>
//#include <components/libraries/crypto/nrf_crypto_hash.h>


#define LOGFirmwareReaderDebug LOGd
#define LOGFirmwareReaderInfo LOGi

#define FIRMWAREREADER_LOG_LVL SERIAL_DEBUG


/**
 * current section to be printed (in parts) for debugging purposes.
 */
static FirmwareSectionInfo firmwareSectionInfo = getFirmwareSectionInfo<FirmwareSection::BootloaderSettings>();

// --------------------------------------------------------------------------------

FirmwareReader::FirmwareReader() :
	firmwarePrinter([this]() {	return this->printRoutine();}),
	firmwareHashPrinter([this]() { return this->printHashRoutine(); }) {

}

cs_ret_code_t FirmwareReader::init() {
	LOGFirmwareReaderInfo("fstorage pointer: 0x%X, section: [0x%08X - 0x%08X]",
			firmwareSectionInfo._fStoragePtr,
			firmwareSectionInfo._addr._start,
			firmwareSectionInfo._addr._end);

	uint32_t nrfCode = nrf_fstorage_init(firmwareSectionInfo._fStoragePtr, &nrf_fstorage_sd, nullptr);

	switch (nrfCode) {
		case NRF_SUCCESS:
			LOGFirmwareReaderInfo("Sucessfully initialized firmwareReader");
			break;
		default:
			LOGw("Failed to initialize firmwareReader. NRF error code %u", nrfCode);
			return ERR_NOT_INITIALIZED;
			break;
	}

	listen();
	return ERR_SUCCESS;
}

void FirmwareReader::read(uint32_t startIndex, uint32_t size, void* data_out) {
	// can verify the output with nrfjprog. E.g.: `nrfjprog --memrd 0x00026000 --w 8 --n 50`

	uint32_t readAddress = firmwareSectionInfo._addr._start + startIndex;

	uint32_t nrfCode = nrf_fstorage_read(
			firmwareSectionInfo._fStoragePtr,
			readAddress,
			data_out,
			size);

	LOGFirmwareReaderDebug("reading %d bytes from address: 0x%x", size, readAddress);

	if(nrfCode != NRF_SUCCESS) {
		LOGFirmwareReaderInfo("failed read, %d", nrfCode);
	}
}

void FirmwareReader::read(uint32_t startIndex, uint32_t size, void* data_out, FirmwareSection section) {
	auto firmwareSectionInfo = getFirmwareSectionInfo(section); // TODO: add boundary check
	read(firmwareSectionInfo._addr._start + startIndex, size, data_out);
}

uint32_t FirmwareReader::printRoutine() {
	constexpr size_t readSize = 128;

	__attribute__((aligned(4))) uint8_t buff[readSize] = {};

	dataoffSet += readSize;

	if(dataoffSet > firmwareSectionInfo._addr._end - firmwareSectionInfo._addr._start) {
		LOGFirmwareReaderDebug("--- section end reached: rolling back to offset = 0 ---");
		dataoffSet = 0;
	}

	read(dataoffSet, readSize, buff, FirmwareSection::BootloaderSettings);

	_logArray(FIRMWAREREADER_LOG_LVL, true, buff, readSize);

	return Coroutine::delayMs(1000);
}

uint32_t FirmwareReader::printHashRoutine(){
	if(CsMath::Decrease(printFirmwareHashCountDown) == 1){
		LOGFirmwareReaderInfo("computing firmware hash");

//		computeHash();

		LOGFirmwareReaderInfo("result of computation:");
	} else {
		LOGFirmwareReaderInfo("not doing firmware hash computation now");
	}
	return Coroutine::delayMs(1000);
}


//void FirmwareReader::computeHash() {
//	LOGFirmwareReaderDebug("Start computing hash.");
//
//	nrf_crypto_hash_sha256_digest_t     m_digest;
//	nrf_crypto_hash_sha256_digest_t     m_digest_swapped;
//	const size_t m_digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;
//
//	nrf_crypto_hash_context_t   hash_context;
//
//	static uint8_t m_message[] = "abc";
//	const size_t  m_message_len = 3; // Skipping null termination
//	uint8_t m_expected_digest[NRF_CRYPTO_HASH_SIZE_SHA256] =
//	{
//	    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
//	    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
//	};
//
//
//    // Initialize crypto subsystem
//    err_code = nrf_crypto_init();
//    APP_ERROR_CHECK(err_code);
//
//
//    NRF_LOG_INFO("Calculating SHA-256 hash with init/update/finalize");
//
//    // Initialize the hash context
//    err_code = nrf_crypto_hash_init(&hash_context, &g_nrf_crypto_hash_sha256_info);
//    APP_ERROR_CHECK(err_code);
//
//    // Run the update function (this can be run multiples of time if the data is accessible
//    // in smaller chunks, e.g. when received on-air.
//    err_code = nrf_crypto_hash_update(&hash_context, m_message, m_message_len);
//    APP_ERROR_CHECK(err_code);
//
//    // Run the finalize when all data has been fed to the update function.
//    // this gives you the result
//    err_code = nrf_crypto_hash_finalize(&hash_context, m_digest, &digest_len);
//    APP_ERROR_CHECK(err_code);
//
//
//    LOGFirmwareReaderDebug("finished computing hash. Result:");
//    _logArray(FIRMWAREREADER_LOG_LVL, true, m_digest, digest_len); // digest_len == NRF_CRYPTO_HASH_SIZE_SHA256?
//    LOGFirmwareReaderDebug("end of result.");

    // ---------------- raw copy of docs below ---------------------

	// https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.0.0/lib_crypto_hash.html
//	Example of hash calculation using init, update, and finalize
//	ret_code_t                                 err_code;
//	static nrf_crypto_hash_context_t           hash_context;
//	static nrf_crypto_hash_sha256_digest_t     hash_digest;
//	uint32_t                                   digest_size = sizeof(hash_digest);
//	// Initialize the hash context
//	err_code = nrf_crypto_hash_init(&hash_context, &g_nrf_crypto_hash_sha256_info);
//	APP_ERROR_CHECK(err_code);
//	// Run the update function (this can be run multiple times if the data
//	// is accessible in smaller chunks, e.g. when received on-air).
//	err_code = nrf_crypto_hash_update(&hash_context, message, message_len);
//	APP_ERROR_CHECK(err_code);
//	// Run the finalize when all data has been fed to the update function.
//	// This gives you the result in hash_digest
//	err_code = nrf_crypto_hash_finalize(&hash_context, hash_digest, &digest_size);
//	APP_ERROR_CHECK(err_code);

	// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.0.0%2Flib_crypto_config.html&anchor=lib_crypto_config_usage_backend
//	Enabling SHA-256 in the nrf_oberon backend
//	// <q> NRF_CRYPTO_BACKEND_OBERON_HASH_SHA256_ENABLED  - Oberon SHA-256 hash functionality
//	// <i> Oberon backend implementation for SHA-256.
//	#ifndef NRF_CRYPTO_BACKEND_OBERON_HASH_SHA256_ENABLED
//	#define NRF_CRYPTO_BACKEND_OBERON_HASH_SHA256_ENABLED 1
//	#endif
//}



void FirmwareReader::handleEvent(event_t& evt) {
	firmwarePrinter.handleEvent(evt);
	firmwareHashPrinter.handleEvent(evt);
}
