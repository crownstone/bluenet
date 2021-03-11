#pragma once

#include <events/cs_EventListener.h>

//// Call loop every 10 ticks. The ticks are every 100 ms so this means every second.
//#define MICROAPP_LOOP_FREQUENCY 10
//
//#define MICROAPP_LOOP_INTERVAL_MS (TICK_INTERVAL_MS * MICROAPP_LOOP_FREQUENCY)

//// Has to match section .firmware_header in linker file nrf_common.ld.
//typedef struct __attribute__((__packed__)) microapp_header_t {
//	uint32_t size;
//	uint32_t checksum;
//	uint32_t offset;
//	uint32_t reserve;
//};

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
	 * The buffer is required to perform writes to flash, as the data has to stay in memory until the  write is done.
	 */
	uint8_t *_buffer = nullptr;

	/**
	 * Erases all pages of the microapp storage.
	 */
	uint16_t erasePages();
};
