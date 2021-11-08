/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_FirmwareReader.h>
#include <dfu/cs_FirmwareSections.h>

#include <logging/cs_Logger.h>


#define LOGFirmwareReaderDebug LOGd
#define LOGFirmwareReaderInfo LOGi

#define FIRMWAREREADER_LOG_LVL SERIAL_DEBUG



static FirmwareSectionInfo firmwareSectionInfo = getFirmwareSectionInfo<FirmwareSection::Mbr>();

// --------------------------------------------------------------------------------

FirmwareReader::FirmwareReader() :
	firmwarePrinter([this]() {
		return this->printRoutine();
}) {

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

void FirmwareReader::read([[maybe_unused]] FirmwareSection section, uint16_t startIndex, uint16_t size, void* data_out) {
	// can verify the output with nrfjprog. E.g.: `nrfjprog --memrd 0x00026000 --w 8 --n 50`

	uint32_t readAddress = firmwareSectionInfo._addr._start + startIndex;

	uint32_t nrfCode = nrf_fstorage_read(
			firmwareSectionInfo._fStoragePtr,
			readAddress,
			data_out,
			size);

	LOGFirmwareReaderDebug("reading %d bytes from address: 0x%x", size, readAddress);

	if(nrfCode != NRF_SUCCESS) {
		LOGFirmwareReaderInfo("failed read");
	}
}

uint32_t FirmwareReader::printRoutine() {
	constexpr size_t readSize = 100;

	__attribute__((aligned(4))) uint8_t buff[readSize] = {};

	dataoffSet += 100;
	read(FirmwareSection::Bluenet, dataoffSet, readSize, buff);

	_logArray(FIRMWAREREADER_LOG_LVL, true, buff, readSize);

	return Coroutine::delayMs(1000);
}

void FirmwareReader::handleEvent(event_t& evt) {
	firmwarePrinter.handleEvent(evt);
}
