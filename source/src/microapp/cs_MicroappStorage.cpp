/*
 * Store "micro" apps in flash.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <nrf_fstorage_sd.h>

#include <algorithm>
#include <ble/cs_UUID.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <microapp/cs_MicroappProtocol.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Crc16.h>
#include <util/cs_Utils.h>

const uint8_t MICROAPP_STORAGE_BUF_SIZE = 32;

void fs_evt_handler_sched(void *data, uint16_t size) {
	nrf_fstorage_evt_t * evt = (nrf_fstorage_evt_t*) data;
	MicroappStorage::getInstance().handleFileStorageEvent(evt);
}

static void fs_evt_handler(nrf_fstorage_evt_t * p_evt) {
	uint32_t retVal = app_sched_event_put(p_evt, sizeof(*p_evt), fs_evt_handler_sched);
	APP_ERROR_CHECK(retVal);
}

NRF_FSTORAGE_DEF(nrf_fstorage_t nrf_microapp_storage) =
{
	.evt_handler    = fs_evt_handler,
	.start_addr     = g_FLASH_MICROAPP_BASE,
	.end_addr       = g_FLASH_MICROAPP_BASE + (0x1000*(g_FLASH_MICROAPP_PAGES)) - 1,
};

MicroappStorage::MicroappStorage() { 
}

cs_ret_code_t MicroappStorage::init() {
	_buffer = new uint8_t[MICROAPP_STORAGE_BUF_SIZE];
	if (_buffer == nullptr) {
		LOGw("Could not allocate buffer");
		return ERR_NO_SPACE;
	}

	uint32_t nrfCode;
	nrfCode = nrf_fstorage_init(&nrf_microapp_storage, &nrf_fstorage_sd, NULL);
	switch (nrfCode) {
		case NRF_SUCCESS:
			LOGi("Sucessfully initialized from 0x%08X to 0x%08X", nrf_microapp_storage.start_addr, nrf_microapp_storage.end_addr);
			break;
		default:
			LOGw("Error code %i", nrfCode);
			return ERR_NOT_INITIALIZED;
			break;
	}

//	uint8_t testData[10] = {0,1,2,3,4,5,6,7,8,9};
//	uint8_t* testDataPtr = testData + 3;
//	uint8_t testDataSize = 4;
//	nrfCode = nrf_fstorage_write(&nrf_microapp_storage, g_FLASH_MICROAPP_BASE, testDataPtr, testDataSize, NULL);
//	LOGi("nrf_fstorage_write data=%u size=%u nrfCode=%u", testDataPtr, testDataSize, nrfCode);
//
//	testDataPtr = testData + 4;
//	testDataSize = 3;
//	nrfCode = nrf_fstorage_write(&nrf_microapp_storage, g_FLASH_MICROAPP_BASE, testDataPtr, testDataSize, NULL);
//	LOGi("nrf_fstorage_write data=%u size=%u nrfCode=%u", testDataPtr, testDataSize, nrfCode);
//
//	testDataPtr = testData + 4;
//	testDataSize = 4;
//	nrfCode = nrf_fstorage_write(&nrf_microapp_storage, g_FLASH_MICROAPP_BASE, testDataPtr, testDataSize, NULL);
//	LOGi("nrf_fstorage_write data=%u size=%u nrfCode=%u", testDataPtr, testDataSize, nrfCode);

	return ERR_SUCCESS;
}

uint16_t MicroappStorage::erasePages() {
	uint32_t err_code;
	err_code = nrf_fstorage_erase(&nrf_microapp_storage, nrf_microapp_storage.start_addr, g_FLASH_MICROAPP_PAGES, NULL);
	return err_code;
}

cs_ret_code_t MicroappStorage::writeChunk(uint8_t appIndex, uint16_t offset, const uint8_t* data, uint16_t size) {
	uint32_t flashAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE + offset;
	LOGi("Write chunk at 0x%08X of size %u", flashAddress, size);

	if (offset + size > MICROAPP_MAX_SIZE) {
		return ERR_NO_SPACE;
	}

	if (size % 4 != 0) {
		LOGw("Chunk size must be multiple of 4B, size=%u", size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// Write chunk in even smaller chunks of MICROAPP_STORAGE_BUF_SIZE.
	// TODO: This could be avoided if `data` would be word aligned,
	// which we could do by making sure the control command payload is word aligned.
	_chunkData = data;
	_chunkSize = size;
	_chunkWritten = 0;
	_chunkFlashAddress = flashAddress;

	return writeNextChunkPart();
}

cs_ret_code_t MicroappStorage::writeNextChunkPart() {
	cs_ret_code_t retCode;

	// Check if done.
	if (_chunkWritten >= _chunkSize) {
		retCode = ERR_SUCCESS;
		LOGi("Chunk written to flash, dispatch event with result %u", retCode);
		resetChunkVars();
		event_t event(CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT, &retCode, sizeof(retCode));
		event.dispatch();
	}

	// Write part of the chunk.
	// Copy to _buffer, so that the data is aligned.
	uint16_t writeSize = std::min((int)MICROAPP_STORAGE_BUF_SIZE, _chunkSize - _chunkWritten);
	memcpy(_buffer, _chunkData + _chunkWritten, writeSize);
	retCode = write(_chunkFlashAddress + _chunkWritten, _buffer, writeSize);
	_chunkWritten += writeSize;

	if (retCode != ERR_SUCCESS) {
		LOGw("Failed to start write to flash, dispatch event with result %u", retCode);
		resetChunkVars();
		event_t event(CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT, &retCode, sizeof(retCode));
		event.dispatch();
	}

	return retCode;
}

cs_ret_code_t MicroappStorage::write(uint32_t flashAddress, const uint8_t* data, uint16_t size) {
	// Only allow 1 write to flash at a time.
	if (_writing) {
		LOGw("Busy writing");
		return ERR_BUSY;
	}

	// Write will only work if the flashAddress, and data pointer are word aligned, and when size is word sized.
	uint32_t nrfCode = nrf_fstorage_write(&nrf_microapp_storage, flashAddress, data, size, NULL);
	switch (nrfCode) {
		case NRF_SUCCESS:
			_writing = true;
			LOGd("Success");
			return ERR_SUCCESS;
		case NRF_ERROR_NO_MEM:
			LOGw("Write queue is full");
			return ERR_BUSY;
		case NRF_ERROR_INVALID_LENGTH:
			LOGw("Invalid length: size=%u", size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		case NRF_ERROR_INVALID_ADDR:
			LOGw("Invalid address: flashAddress=0x%08X data=0x%08X");
			return ERR_NOT_ALIGNED;
		default:
			LOGw("Error %u", nrfCode);
			return ERR_UNSPECIFIED;
	}
}

void MicroappStorage::onFlashWritten(cs_ret_code_t retCode) {
	LOGi("Flash written");
	_writing = false;
	if (retCode == ERR_SUCCESS) {
		writeNextChunkPart();
	}
	else {
		resetChunkVars();
		LOGw("Failed to complete write to flash, dispatch event with result %u", retCode);
		event_t event(CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT, &retCode, sizeof(retCode));
		event.dispatch();
	}
}

void MicroappStorage::resetChunkVars() {
	_chunkData = nullptr;
	_chunkSize = 0;
	_chunkWritten = 0;
	_chunkFlashAddress = 0;
}



void MicroappStorage::getAppHeader(uint8_t appIndex, microapp_binary_header_t* header) {
	LOGd("Get app header");
	const uint32_t addr = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	const uint8_t size = sizeof(*header);

	nrf_fstorage_read(&nrf_microapp_storage, addr, header, size);
	printHeader(SERIAL_DEBUG, header);
}

uint32_t MicroappStorage::getStartInstructionAddress(uint8_t appIndex) {
	uint32_t startAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	microapp_binary_header_t header;
	getAppHeader(appIndex, &header);

	startAddress += header.startOffset;
	return startAddress;
}

cs_ret_code_t MicroappStorage::validateApp(uint8_t appIndex) {
	LOGd("Validate app %u", appIndex);

	microapp_binary_header_t header;
	getAppHeader(appIndex, &header);

	if (header.size > MICROAPP_MAX_SIZE) {
		LOGw("Microapp size=%u is too large", header.size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	uint32_t startAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	uint32_t endAddress = startAddress + header.size;
	LOGd("startAddress=0x%08X endAddress=0x%08X", startAddress, endAddress);

	// Check if the address of first instruction is in the binary.
	if (startAddress + header.startOffset >= endAddress) {
		LOGw("Microapp startOffset=%u is too large.", header.startOffset);
		return ERR_WRONG_PARAMETER;
	}

	// Compare header checksum.
	uint16_t checksumHeader = header.checksumHeader;
	header.checksumHeader = 0;
	uint16_t crc = crc16(reinterpret_cast<uint8_t*>(&header), sizeof(header));
	LOGi("Header checksum: expected=%u calculated=%u", checksumHeader, crc);
	if (checksumHeader != crc) {
		_logArray(SERIAL_DEBUG, true, reinterpret_cast<uint8_t*>(&header), sizeof(header));
		return ERR_MISMATCH;
	}

	// Compare binary checksum.
	// Calculate checksum in chunks, so that we don't have to load the whole binary in ram.
	const uint32_t bufSize = MICROAPP_STORAGE_BUF_SIZE; // Can be any size.
	uint8_t buf[bufSize];
	uint32_t nrfCode;

	// Init the CRC.
	crc = crc16(nullptr, 0);

	uint32_t binStartAddress = startAddress + sizeof(header);
	uint16_t binSize = header.size - sizeof(header);
	LOGd("binStartAddress=0x%08X binSize=%u sizeof(header)=%u", binStartAddress, binSize, sizeof(header));

//	endAddress = CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(endAddress, 4);
	for (uint32_t flashAddress = binStartAddress; flashAddress < endAddress; flashAddress += bufSize) {
		uint16_t readSize = std::min(bufSize, endAddress - flashAddress);
		LOGv("nrf_fstorage_read 0x%08X 0x%X %u", flashAddress, buf, readSize);
		nrfCode = nrf_fstorage_read(&nrf_microapp_storage, flashAddress, buf, readSize);
		if (nrfCode != NRF_SUCCESS) {
			LOGw("Error reading fstorage: %u. flashAddress=0x%08X buf=0x%X readSize=%u", nrfCode, flashAddress, buf, readSize);
			return ERR_READ_FAILED;
		}
		crc = crc16(buf, readSize, &crc);
	}

	LOGi("Binary checksum: expected=%u calculated=%u", header.checksum, crc);
	if (header.checksum != crc) {
		return ERR_MISMATCH;
	}
	return ERR_SUCCESS;
}

void MicroappStorage::printHeader(uint8_t logLevel, microapp_binary_header_t* header) {
	_log(logLevel, true, "sdkVersion=%u.%u size=%u checksum=%u checksumHeader=%u appBuildVersion=%u startOffset=%u ",
			header->sdkVersionMajor,
			header->sdkVersionMinor,
			header->size,
			header->checksum,
			header->checksumHeader,
			header->appBuildVersion,
			header->startOffset);
}

/**
 * Return result of fstorage operation to sender. We only send this event after fstorage returns to pace the 
 * incoming messages. In this way it is also possible to resend a particular chunk.
 */
void MicroappStorage::handleFileStorageEvent(nrf_fstorage_evt_t *evt) {
	cs_ret_code_t retCode = ERR_SUCCESS;
	switch (evt->result) {
		case NRF_SUCCESS: {
			retCode = ERR_SUCCESS;
			break;
		}
		default: {
			LOGe("Flash error: %u", evt->result);
			retCode = ERR_UNSPECIFIED;
		}
	}
	switch (evt->id) {
		case NRF_FSTORAGE_EVT_WRITE_RESULT: {
			onFlashWritten(retCode);
			break;
		}
		case NRF_FSTORAGE_EVT_ERASE_RESULT: {
			LOGi("Flash erased");
			break;
		}
		default:
			break;
	}
}
