#pragma once

#include <events/cs_EventListener.h>

/**
 * The class MicroappStorage has functionality to store a second app (and perhaps in the future even more apps) on
 * another part of the flash memory.
 */
class MicroappStorage {
public:
	static MicroappStorage& getInstance() {
		static MicroappStorage instance;
		return instance;
	}

	/**
	 * Initialize fstorage, allocate buffer.
	 */
	cs_ret_code_t init();

	/**
	 * Get a copy of the microapp binary header.
	 */
	void getAppHeader(uint8_t appIndex, microapp_binary_header_t* header);

	/**
	 * Get the address in flash, where the microapp program starts.
	 */
	uint32_t getStartInstructionAddress(uint8_t appIndex);

	/**
	 * Validate the overall binary, this goes through flash and checks it completely.
	 * All flash write operations have to have finished before.
	 */
	cs_ret_code_t validateApp(uint8_t appIndex);

	/**
	 * Write a chunk to flash.
	 */
	cs_ret_code_t writeChunk(uint8_t appIndex, uint16_t offset, const uint8_t* data, uint16_t size);

	void printHeader(uint8_t logLevel, microapp_binary_header_t* header);

	/**
	 * Internal usage: when fstorage is done, this function will be called (indirectly through app_scheduler).
	 */
	void handleFileStorageEvent(nrf_fstorage_evt_t *evt);
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappStorage();
	MicroappStorage(MicroappStorage const&);
	void operator=(MicroappStorage const &);

	/**
	 * Keep up whether or not we are currently writing to flash.
	 */
	bool _writing = false;

	/**
	 * The buffer is required to perform writes to flash, as the data has to stay in memory until the  write is done.
	 */
	uint8_t *_buffer = nullptr;

	/**
	 * When writing a chunk of data, it will be done in parts.
	 * These variable are needed to know what to write where to.
	 */
	const uint8_t* _chunkData = nullptr;
	uint16_t _chunkSize = 0;
	uint16_t _chunkWritten = 0;
	uint32_t _chunkFlashAddress = 0;

	cs_ret_code_t writeNextChunkPart();

	cs_ret_code_t write(uint32_t flashAddress, const uint8_t* data, uint16_t size);

	void onFlashWritten(cs_ret_code_t retCode);

	/**
	 * To be called when a chunk has been written (or when it failed to write).
	 * Resets all variables.
	 */
	void resetChunkVars();

	/**
	 * Erases all pages of the microapp storage.
	 */
	uint16_t erasePages();
};
