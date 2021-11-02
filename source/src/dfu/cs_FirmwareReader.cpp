/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_FirmwareReader.h>
#include <logging/cs_Logger.h>

#include <nrf_fstorage_sd.h>

#define LOGFirmwareReaderDebug LOGd
#define LOGFirmwareReaderInfo LOGi

#define FIRMWAREREADER_LOG_LVL SERIAL_DEBUG



void firmwareReaderFsEventHandler(nrf_fstorage_evt_t * p_evt)  {
	LOGd("firmware reader fs event handler called");
}

// --------------------------------------------------------------------------------
// -------------------- Memory region definitions for fstorage --------------------
// --------------------------------------------------------------------------------

// TODO: change to whole flash mem for ease of use.

// ------- Bluenet -------
//NRF_FSTORAGE_DEF(nrf_fstorage_t	firmwareReaderFsInstance) = {
//	.evt_handler    = firmwareReaderFsEventHandler,
//	.start_addr     = g_APPLICATION_START_ADDRESS,
//	.end_addr       = g_APPLICATION_START_ADDRESS + g_APPLICATION_LENGTH,
//};


// ------- Bootloader -------
//NRF_FSTORAGE_DEF(nrf_fstorage_t	firmwareReaderFsInstance) = {
//	.evt_handler    = firmwareReaderFsEventHandler,
//	.start_addr     = 0x71000,
//	.end_addr       = 0x71000 + 0xD000,
//};

// ------- MBR -------
extern int __start_mbr_params_page, __stop_mbr_params_page;
static const uint32_t start_mbr_params_page = static_cast<uint32_t>(__start_mbr_params_page);
static const uint32_t stop_mbr_params_page = static_cast<uint32_t>(__stop_mbr_params_page);

NRF_FSTORAGE_DEF(nrf_fstorage_t	firmwareReaderFsInstance) = {
	.evt_handler    = firmwareReaderFsEventHandler,
	.start_addr     = start_mbr_params_page,
	.end_addr       = stop_mbr_params_page,
};


// --------------------------------------------------------------------------------

FirmwareReader::FirmwareReader() :
	firmwarePrinter([this]() {
		return this->printRoutine();
}) {

}

cs_ret_code_t FirmwareReader::init() {

	uint32_t nrfCode = nrf_fstorage_init(&firmwareReaderFsInstance, &nrf_fstorage_sd, nullptr);

	switch (nrfCode) {
		case NRF_SUCCESS:
			LOGFirmwareReaderInfo(
					"Sucessfully initialized firmwareReader 0x%08X - 0x%08X",
					firmwareReaderFsInstance.start_addr,
					firmwareReaderFsInstance.end_addr);
			break;
		default:
			LOGw("Failed to initialize firmwareReader. NRF error code %u", nrfCode);
			return ERR_NOT_INITIALIZED;
			break;
	}

	listen();
	return ERR_SUCCESS;
}

void FirmwareReader::read(FirmwareSection section, uint16_t startIndex, uint16_t size, void* data_out) {
	// can check with this for equality:
	// nrfjprog --memrd 0x00026000 --w 8 --n 50
//	uint32_t nrfCode = nrf_fstorage_read(&firmwareReaderFsInstance, g_APPLICATION_START_ADDRESS, data_out, size);

	[[maybe_unused]] uint32_t applicationStartAddress = g_APPLICATION_START_ADDRESS;
	[[maybe_unused]] uint32_t bootloaderStartAddress = 0x71000;
	[[maybe_unused]] uint32_t mbrAddress = start_mbr_params_page;

	uint32_t readAddress = mbrAddress;

	LOGFirmwareReaderDebug("reading %d bytes from address: 0x%x", size, readAddress + startIndex);
	uint32_t nrfCode = nrf_fstorage_read(&firmwareReaderFsInstance, readAddress + startIndex, data_out, size);


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
