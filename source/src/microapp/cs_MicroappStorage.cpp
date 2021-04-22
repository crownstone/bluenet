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

#define LOGMicroappInfo LOGi
#define LOGMicroappDebug LOGnone
#define LOGMicroappVerboseLevel SERIAL_VERBOSE

void fs_evt_handler_sched(void *data, uint16_t size) {
	nrf_fstorage_evt_t * evt = (nrf_fstorage_evt_t*) data;
	MicroappStorage::getInstance().handleFileStorageEvent(evt);
}

// Event handler of our nrf_fstorage instance.
static void fs_evt_handler(nrf_fstorage_evt_t * p_evt) {
	// TODO: do we need to schedule these events? They might already be called from app scheduler, depending on sdk_config.
	uint32_t retVal = app_sched_event_put(p_evt, sizeof(*p_evt), fs_evt_handler_sched);
	APP_ERROR_CHECK(retVal);
}

NRF_FSTORAGE_DEF(nrf_fstorage_t nrf_microapp_storage) =
{
	.evt_handler    = fs_evt_handler,
	.start_addr     = g_FLASH_MICROAPP_BASE,
	.end_addr       = g_FLASH_MICROAPP_BASE + (CS_FLASH_PAGE_SIZE * g_FLASH_MICROAPP_PAGES) - 1,
};

MicroappStorage::MicroappStorage() {
}

cs_ret_code_t MicroappStorage::init() {
	uint32_t nrfCode;
	nrfCode = nrf_fstorage_init(&nrf_microapp_storage, &nrf_fstorage_sd, nullptr);
	switch (nrfCode) {
		case NRF_SUCCESS:
			LOGMicroappInfo("Sucessfully initialized from 0x%08X to 0x%08X", nrf_microapp_storage.start_addr, nrf_microapp_storage.end_addr);
			break;
		default:
			LOGw("NRF error code %u", nrfCode);
			return ERR_NOT_INITIALIZED;
			break;
	}


	return ERR_SUCCESS;
}

cs_ret_code_t MicroappStorage::erase(uint8_t appIndex) {
	if (_writing) {
		return ERR_BUSY;
	}

	if (isErased(appIndex)) {
		return ERR_SUCCESS;
	}

	uint32_t flashAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	LOGMicroappInfo("erase addr=0x%08X size=%u", flashAddress, MICROAPP_MAX_SIZE / CS_FLASH_PAGE_SIZE);
	uint32_t nrfCode = nrf_fstorage_erase(&nrf_microapp_storage, flashAddress, MICROAPP_MAX_SIZE / CS_FLASH_PAGE_SIZE, nullptr);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Failed to start erase: %u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t MicroappStorage::writeChunk(uint8_t appIndex, uint16_t offset, const uint8_t* data, uint16_t size) {
	uint32_t flashAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE + offset;
	LOGMicroappInfo("Write chunk at 0x%08X of size %u", flashAddress, size);

	if (offset + size > MICROAPP_MAX_SIZE) {
		return ERR_NO_SPACE;
	}

	if (size % 4 != 0) {
		LOGw("Chunk size must be multiple of 4B, size=%u", size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	if (!isErased(flashAddress, size)) {
		LOGw("Chunk is not erased");
		return ERR_WRITE_DISABLED;
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
	LOGMicroappDebug("writeNextChunkPart _chunkSize=%u _chunkWritten=%u", _chunkSize, _chunkWritten);
	cs_ret_code_t retCode;

	// Check if done.
	if (_chunkWritten >= _chunkSize) {
		LOGMicroappInfo("Done");
		return ERR_SUCCESS;
	}

	// Write part of the chunk.
	// Copy to _buffer, so that the data is aligned.
	uint16_t writeSize = std::min((int)MICROAPP_STORAGE_BUF_SIZE, _chunkSize - _chunkWritten);
	memcpy(_writeBuffer, _chunkData + _chunkWritten, writeSize);
	retCode = write(_chunkFlashAddress + _chunkWritten, _writeBuffer, writeSize);
	_chunkWritten += writeSize;

	if (retCode != ERR_SUCCESS) {
		LOGw("Failed to start write to flash, dispatch event with result %u", retCode);
		return retCode;
	}
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t MicroappStorage::write(uint32_t flashAddress, const uint8_t* data, uint16_t size) {
	LOGMicroappDebug("write %u bytes from 0x%X to 0x%08X", size, data, flashAddress);
	_logArray(LOGMicroappVerboseLevel, true, data, size);
	// Only allow 1 write to flash at a time.
	if (_writing) {
		LOGw("Busy writing");
		return ERR_BUSY;
	}

	// Write will only work if the flashAddress, and data pointer are word aligned, and when size is word sized.
	uint32_t nrfCode = nrf_fstorage_write(&nrf_microapp_storage, flashAddress, data, size, nullptr);
	switch (nrfCode) {
		case NRF_SUCCESS:
			_writing = true;
			LOGMicroappDebug("Success");
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
	LOGMicroappDebug("onFlashWritten retCode=%u", retCode);
	_writing = false;
	if (retCode != ERR_SUCCESS) {
		resetChunkVars();
		LOGw("Failed to complete write to flash, dispatch event with result %u", retCode);
		event_t event(CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT, &retCode, sizeof(retCode));
		event.dispatch();
	}

	retCode = writeNextChunkPart();
	if (retCode == ERR_WAIT_FOR_SUCCESS) {
		// Wait for next written event.
		return;
	}

	// Write is done or there was an error.
	resetChunkVars();
	event_t event(CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT, &retCode, sizeof(retCode));
	event.dispatch();
}

void MicroappStorage::resetChunkVars() {
	LOGMicroappDebug("resetChunkVars");
	_chunkData = nullptr;
	_chunkSize = 0;
	_chunkWritten = 0;
	_chunkFlashAddress = 0;
}



void MicroappStorage::getAppHeader(uint8_t appIndex, microapp_binary_header_t& header) {
	LOGMicroappDebug("Get app header");
	const uint32_t addr = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	const uint8_t size = sizeof(header);

	LOGMicroappDebug("read %u bytes from 0x%08X to 0x%X", size, addr, header);
	uint32_t nrfCode = nrf_fstorage_read(&nrf_microapp_storage, addr, &header, size);
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to read app header");
	}
	printHeader(LOGMicroappVerboseLevel, header);
}

uint32_t MicroappStorage::getStartInstructionAddress(uint8_t appIndex) {
	uint32_t startAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	microapp_binary_header_t header;
	getAppHeader(appIndex, header);

	startAddress += header.startOffset;
	return startAddress;
}

bool MicroappStorage::isErased(uint8_t appIndex) {
	const uint32_t addr = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	return isErased(addr, MICROAPP_MAX_SIZE);
}

bool MicroappStorage::isErased(uint32_t flashAddress, uint16_t size) {
	const uint32_t bufSize = MICROAPP_STORAGE_BUF_SIZE; // Can be any multiple of 4.
	uint8_t readBuf[bufSize];
	uint8_t emptyBuf[bufSize];
	memset(emptyBuf, 0xFF, bufSize);

	uint32_t nrfCode;
	uint32_t endAddress = flashAddress + size;
	for (uint32_t addr = flashAddress; addr < endAddress; addr += bufSize) {
		uint16_t readSize = std::min(bufSize, endAddress - addr);
		_log(LOGMicroappVerboseLevel, true, "read %u bytes from 0x%08X to 0x%X", readSize, addr, readBuf);
		nrfCode = nrf_fstorage_read(&nrf_microapp_storage, addr, readBuf, readSize);
		_logArray(LOGMicroappVerboseLevel, true, readBuf, readSize);
		if (nrfCode != NRF_SUCCESS) {
			LOGw("Error reading fstorage: %u. flashAddress=0x%08X buf=0x%X readSize=%u", nrfCode, flashAddress, readBuf, readSize);
			return false;
		}
		if (memcmp(readBuf, emptyBuf, bufSize) != 0) {
			return false;
		}
	}
	return true;
}

cs_ret_code_t MicroappStorage::validateApp(uint8_t appIndex) {
	LOGMicroappInfo("Validate app %u", appIndex);

	microapp_binary_header_t header;
	getAppHeader(appIndex, header);

	if (header.size > MICROAPP_MAX_SIZE) {
		LOGw("Microapp size=%u is too large", header.size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	uint32_t startAddress = nrf_microapp_storage.start_addr + appIndex * MICROAPP_MAX_SIZE;
	uint32_t endAddress = startAddress + header.size;
	LOGMicroappDebug("startAddress=0x%08X endAddress=0x%08X", startAddress, endAddress);

	// Check if the address of first instruction is in the binary.
	if (startAddress + header.startOffset >= endAddress) {
		LOGw("Microapp startOffset=%u is too large.", header.startOffset);
		return ERR_WRONG_PARAMETER;
	}

	// Compare header checksum.
	uint16_t checksumHeader = header.checksumHeader;
	header.checksumHeader = 0;
	uint16_t crc = crc16(reinterpret_cast<uint8_t*>(&header), sizeof(header));
	LOGMicroappInfo("Header checksum: expected=%u calculated=%u", checksumHeader, crc);
	if (checksumHeader != crc) {
		_logArray(SERIAL_DEBUG, true, reinterpret_cast<uint8_t*>(&header), sizeof(header));
		return ERR_MISMATCH;
	}

	// Compare binary checksum.
	// Calculate checksum in chunks, so that we don't have to load the whole binary in ram.
	const uint32_t bufSize = MICROAPP_STORAGE_BUF_SIZE; // Can be any multiple of 4.
	uint8_t buf[bufSize];
	uint32_t nrfCode;

	// Init the CRC.
	crc = crc16(nullptr, 0);

	uint32_t binStartAddress = startAddress + sizeof(header);
	LOGMicroappDebug("binStartAddress=0x%08X", binStartAddress);

//	endAddress = CS_ROUND_UP_TO_MULTIPLE_OF_POWER_OF_2(endAddress, 4);
	for (uint32_t flashAddress = binStartAddress; flashAddress < endAddress; flashAddress += bufSize) {
		uint16_t readSize = std::min(bufSize, endAddress - flashAddress);
		_log(LOGMicroappVerboseLevel, true, "read %u bytes from 0x%08X to 0x%X", readSize, flashAddress, buf);
		nrfCode = nrf_fstorage_read(&nrf_microapp_storage, flashAddress, buf, readSize);
		if (nrfCode != NRF_SUCCESS) {
			LOGw("Error reading fstorage: %u. flashAddress=0x%08X buf=0x%X readSize=%u", nrfCode, flashAddress, buf, readSize);
			return ERR_READ_FAILED;
		}
		crc = crc16(buf, readSize, &crc);
	}

	LOGMicroappInfo("Binary checksum: expected=%u calculated=%u", header.checksum, crc);
	if (header.checksum != crc) {
		return ERR_MISMATCH;
	}
	return ERR_SUCCESS;
}

void MicroappStorage::printHeader(uint8_t logLevel, microapp_binary_header_t& header) {
#if CS_SERIAL_NRF_LOG_ENABLED == 0
	_log(logLevel, true, "sdkVersion=%u.%u size=%u checksum=%u checksumHeader=%u appBuildVersion=%u startOffset=%u ",
			header.sdkVersionMajor,
			header.sdkVersionMinor,
			header.size,
			header.checksum,
			header.checksumHeader,
			header.appBuildVersion,
			header.startOffset);
#endif
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
			LOGMicroappDebug("Write result addr=0x%08X len=%u src=0x%X", evt->addr, evt->len, evt->p_src);
			_logArray(LOGMicroappVerboseLevel, true, (const uint8_t*)(evt->p_src), evt->len);
			onFlashWritten(retCode);
			break;
		}
		case NRF_FSTORAGE_EVT_ERASE_RESULT: {
			LOGMicroappInfo("Flash erase result=%u addr=0x%08X len=%u", evt->result, evt->addr, evt->len);
			event_t event(CS_TYPE::EVT_MICROAPP_ERASE_RESULT, &retCode, sizeof(retCode));
			event.dispatch();
			break;
		}
		default:
			break;
	}
}
