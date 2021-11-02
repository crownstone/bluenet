/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_FirmwareReader.h>
#include <logging/cs_Logger.h>

#define LOGFirmwareReaderDebug LOGd

#define FIRMWAREREADER_LOG_LVL SERIAL_DEBUG


FirmwareReader::FirmwareReader() :
	firmwarePrinter([this]() {
		return this->printRoutine();
}) {

}

cs_ret_code_t FirmwareReader::init() {
	listen();
	return ERR_SUCCESS;
}

void FirmwareReader::read(
		[[maybe_unused]] FirmwareSection section,
		[[maybe_unused]] uint16_t startIndex,
		[[maybe_unused]] uint16_t size,
		[[maybe_unused]] const void* data_out) {
	//	section;
	//	size;
	//	data_out;
}

uint32_t FirmwareReader::printRoutine() {
	constexpr size_t readSize = 50;

	uint8_t buff[readSize] = {};
	read(FirmwareSection::Bluenet, 0, readSize, buff);
	LOGFirmwareReaderDebug("firmware reader tick");
	_logArray(FIRMWAREREADER_LOG_LVL, true, buff, readSize);

	return Coroutine::delayMs(1000);
}

void FirmwareReader::handleEvent(event_t& evt) {
	firmwarePrinter.handleEvent(evt);

}
