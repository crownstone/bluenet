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

	uint8_t testData[10] = {0,1,2,3,4,5,6,7,8,9};
	nrfCode = nrf_fstorage_write(&nrf_microapp_storage, g_FLASH_MICROAPP_BASE, testData + 3, 4, NULL);
	LOGi(nrfCode);

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

//	// Make a copy of the data
//	if (size > MICROAPP_STORAGE_BUF_SIZE) {
//		return ERR_BUFFER_TOO_SMALL;
//	}
//	memcpy(_buffer, data, size);

	uint32_t nrfCode = nrf_fstorage_write(&nrf_microapp_storage, flashAddress, data, size, NULL);
	switch (nrfCode) {
		case NRF_SUCCESS:
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

void MicroappStorage::getAppHeader(uint8_t appIndex, microapp_binary_header_t* header) {
	LOGd("Get app header");
	const uint32_t addr = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	const uint8_t size = sizeof(*header);

	nrf_fstorage_read(&nrf_microapp_storage, addr, header, size);
	printHeader(SERIAL_DEBUG, header);
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
	if (header.startAddress > endAddress - sizeof(header.startAddress)) {
		LOGw("Microapp startAddress=0x%08X is too large.", header.startAddress);
		return ERR_WRONG_PARAMETER;
	}

	// Compare header checksum.
	uint16_t checksumHeader = header.checksumHeader;
	header.checksum = 0;
	uint16_t crc = crc16(reinterpret_cast<uint8_t*>(&header), sizeof(header));
	LOGi("Header checksum: expected=%u calculated=%u", checksumHeader, crc);
	if (checksumHeader != crc) {
		return ERR_MISMATCH;
	}

	// Compare binary checksum.
	// Calculate checksum in chunks, so that we don't have to load the whole binary in ram.
	const uint32_t bufSize = MICROAPP_STORAGE_BUF_SIZE;
	uint8_t buf[bufSize];
	uint32_t nrfCode;

	// Init the CRC.
	crc = crc16(nullptr, 0);

	uint32_t binStartAddress = startAddress + sizeof(header);
	uint16_t binSize = header.size - sizeof(header);

	// First read whole chunks.
	uint16_t wholeChunkCount = binSize / bufSize;
	uint32_t flashAddress = binStartAddress;
	for (uint16_t i = 0; i < wholeChunkCount; ++i) {
		nrfCode = nrf_fstorage_read(&nrf_microapp_storage, flashAddress, buf, bufSize);
		if (nrfCode != ERR_SUCCESS) {
			LOGw("Error reading fstorage: %u", nrfCode);
			return ERR_READ_FAILED;
		}
		crc16(buf, bufSize, &crc);
		flashAddress += bufSize;
	}
	// Then read the remainder.
	uint16_t remainder = binSize - wholeChunkCount * bufSize;
	if (remainder) {
		nrfCode = nrf_fstorage_read(&nrf_microapp_storage, flashAddress, buf, remainder);
		if (nrfCode != ERR_SUCCESS) {
			LOGw("Error reading fstorage: %u", nrfCode);
			return ERR_READ_FAILED;
		}
		crc16(buf, remainder, &crc);
	}
	LOGd("crc=%u", crc);

	// Or in one loop.
	crc = crc16(nullptr, 0);
	for (uint16_t flashAddress = binStartAddress; flashAddress < endAddress; flashAddress += bufSize) {
		uint16_t readSize = std::min(bufSize, endAddress - flashAddress);
		nrfCode = nrf_fstorage_read(&nrf_microapp_storage, flashAddress, buf, readSize);
		if (nrfCode != ERR_SUCCESS) {
			LOGw("Error reading fstorage: %u", nrfCode);
			return ERR_READ_FAILED;
		}
		crc16(buf, readSize, &crc);
	}
	LOGd("crc=%u", crc);

	LOGi("Binary checksum: expected=%u calculated=%u", header.checksum, crc);
	if (header.checksum != crc) {
		return ERR_MISMATCH;
	}
	return ERR_SUCCESS;
}

void MicroappStorage::printHeader(uint8_t logLevel, microapp_binary_header_t* header) {
	_log(logLevel, true, "startAddress=%u sdkVersion=%u.%u size=%u checksum=%u checksumHeader=%u appBuildVersion=%u",
			header->startAddress,
			header->sdkVersionMajor,
			header->sdkVersionMinor,
			header->size,
			header->checksum,
			header->checksumHeader,
			header->appBuildVersion);
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
			LOGi("Flash written");
			event_t event(CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT, &retCode, sizeof(retCode));
			event.dispatch();
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
