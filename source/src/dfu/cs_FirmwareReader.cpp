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


// --------------------------------------------------------------------------------

FirmwareReader::FirmwareReader() :
	firmwarePrinter(/*[this]() {	return this->printRoutine();}*/),
	firmwareHashPrinter(/*[this]() { return this->printHashRoutine(); }*/) {

}


uint32_t FirmwareReader::initFStorage(FirmwareSection sect) {
	auto sectionInfo = getFirmwareSectionInfo(sect);
	LOGFirmwareReaderInfo("initializing fstorage pointer: 0x%X, section: [0x%08X - 0x%08X]",
				sectionInfo._fStoragePtr,
				sectionInfo._addr._start,
				sectionInfo._addr._end);

	return nrf_fstorage_init(sectionInfo._fStoragePtr, &nrf_fstorage_sd, nullptr);
}

cs_ret_code_t FirmwareReader::init() {
	for (auto section : {FirmwareSection::MicroApp /*, FirmwareSection::Ipc */}) {
		uint32_t nrfCode = initFStorage(section);

		if(nrfCode != NRF_SUCCESS) {
			LOGw("Failed to initialize firmwareReader section %u. NRF error code %u",
					static_cast<uint8_t>(section),
					nrfCode);
			return ERR_NOT_INITIALIZED;
		} else {
			LOGFirmwareReaderInfo("success");
		}
	}

	listen();
	return ERR_SUCCESS;
}

uint32_t FirmwareReader::read(uint32_t startIndex, uint32_t size, void* data_out, FirmwareSection section) {
	// can verify the output with nrfjprog. E.g.: `nrfjprog --memrd 0x00026000 --w 8 --n 50`
	LOGFirmwareReaderDebug("reading from section %u", static_cast<uint8_t>(section));

	auto firmwareSectionInfo = getFirmwareSectionInfo(section);

	uint32_t readAddress = firmwareSectionInfo._addr._start + startIndex;
	// TODO: add boundary check

	uint32_t nrfCode = nrf_fstorage_read(
			firmwareSectionInfo._fStoragePtr,
			firmwareSectionInfo._addr._start + startIndex,
			data_out,
			size);

	LOGFirmwareReaderDebug("reading %u bytes from address: 0x%x", size, readAddress);

	if(nrfCode != NRF_SUCCESS) {
		LOGFirmwareReaderInfo("failed read, %d", nrfCode);
	}

	return nrfCode;
}

//uint32_t FirmwareReader::printRoutine() {
//	constexpr size_t readSize = 128;
//
//	__attribute__((aligned(4))) uint8_t buff[readSize] = {};
//
//	dataoffSet += readSize;
//
//	if(dataoffSet > firmwareSectionInfo._addr._end - firmwareSectionInfo._addr._start) {
//		LOGFirmwareReaderDebug("--- section end reached: rolling back to offset = 0 ---");
//		dataoffSet = 0;
//	}
//
//	read(dataoffSet, readSize, buff, FirmwareSection::BootloaderSettings);
//
//	_logArray(FIRMWAREREADER_LOG_LVL, true, buff, readSize);
//
//	return Coroutine::delayMs(1000);
//}

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


void FirmwareReader::handleEvent(event_t& evt) {
//	firmwarePrinter.handleEvent(evt);
//	firmwareHashPrinter.handleEvent(evt);
}
