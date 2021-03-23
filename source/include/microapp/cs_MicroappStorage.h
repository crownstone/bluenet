#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_MicroappPackets.h>
#include <ble/cs_Nordic.h> // TODO: don't use nrf_fstorage_evt_t in header.

constexpr uint8_t MICROAPP_STORAGE_BUF_SIZE = 32;

/**
 * Class to store microapps on flash.
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
	 *
	 * @param[in] appIndex   Index of the microapp, validity is not checked.
	 * @param[out] header    Header to read to.
	 */
	void getAppHeader(uint8_t appIndex, microapp_binary_header_t& header);

	/**
	 * Get the address in flash, where the microapp program starts.
	 *
	 * @param[in] appIndex   Index of the microapp, validity is not checked.
	 */
	uint32_t getStartInstructionAddress(uint8_t appIndex);

	/**
	 * Checks if storage space of this microapp is erased.
	 *
	 * @param[in] appIndex   Index of the microapp, validity is not checked.
	 * @return true                         Storage space of this app is erased, and thus ready to be written.
	 */
	bool isErased(uint8_t appIndex);

	/**
	 * Erases storage space of given app.
	 * First checks if storage is already erased.
	 *
	 * @param[in] appIndex   Index of the microapp, validity is not checked.
	 *
	 * @return ERR_SUCCESS_NO_CHANGE        The storage is already erased.
	 * @return ERR_WAIT_FOR_SUCCESS         The storage will be written erased, wait for EVT_MICROAPP_ERASE_RESULT.
	 */
	cs_ret_code_t erase(uint8_t appIndex);

	/**
	 * Write a chunk to flash.
	 *
	 * @param[in] appIndex   Index of the microapp, validity is not checked.
	 * @param[in] offset     Offset of the data in bytes from the start of the app storage space.
	 * @param[in] data       Pointer to the data to be written. Must remain valid until the write is finished.
	 * @param[in] size       Size of the data to be written, must be a multiple of 4.
	 *
	 * @return ERR_SUCCESS_NO_CHANGE        The data is already on flash.
	 * @return ERR_WAIT_FOR_SUCCESS         The data will be written to flash, wait for EVT_MICROAPP_UPLOAD_RESULT.
	 * @return ERR_NO_SPACE                 Data would go outside the app storage space.
	 * @return ERR_WRONG_PAYLOAD_LENGTH     Data size is not a multiple of 4.
	 * @return ERR_WRITE_DISABLED           App storage space is not erased.
	 * @return ERR_BUSY                     Another chunk is being written already.
	 */
	cs_ret_code_t writeChunk(uint8_t appIndex, uint16_t offset, const uint8_t* data, uint16_t size);

	/**
	 * Validate the overall binary, this goes through flash and checks it completely.
	 * All flash write operations have to have finished before.
	 *
	 * @param[in] appIndex   Index of the microapp, validity is not checked.
	 * @return ERR_SUCCESS                  The app binary header is valid, and the checksums match.
	 */
	cs_ret_code_t validateApp(uint8_t appIndex);

	void printHeader(uint8_t logLevel, microapp_binary_header_t& header);

	/**
	 * Internal usage: when fstorage is done, this function will be called (indirectly through app_scheduler).
	 */
	void handleFileStorageEvent(nrf_fstorage_evt_t *evt);
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappStorage();
	MicroappStorage(MicroappStorage const&) = delete;
	void operator=(MicroappStorage const &)  = delete;

	/**
	 * Keep up whether or not we are currently writing to flash.
	 */
	bool _writing = false;

	/**
	 * The buffer is required to perform writes to flash, as the data has to
	 * be aligned, and stay in memory until the write is done.
	 */
	__attribute__((aligned(4)))	uint8_t _writeBuffer[MICROAPP_STORAGE_BUF_SIZE];

	/**
	 * When writing a chunk of data, it will be done in parts.
	 * These variable are needed to know what to write where to.
	 */
	const uint8_t* _chunkData = nullptr;
	uint16_t _chunkSize = 0;
	uint16_t _chunkWritten = 0;
	uint32_t _chunkFlashAddress = 0;

	/**
	 * Write the next part of a chunk.
	 * Called first time from command, and every time when a flash write is done.
	 *
	 * @return ERR_SUCCESS                  Complete chunk has been written to flash.
	 * @return ERR_WAIT_FOR_SUCCESS         The data will be written to flash.
	 */
	cs_ret_code_t writeNextChunkPart();

	/**
	 * Write to flash.
	 *
	 * @return ERR_SUCCESS                  The data will be written to flash.
	 */
	cs_ret_code_t write(uint32_t flashAddress, const uint8_t* data, uint16_t size);

	/**
	 * Called when data has been written to flash.
	 */
	void onFlashWritten(cs_ret_code_t retCode);

	/**
	 * To be called when a chunk has been written (or when it failed to write).
	 * Resets all variables.
	 */
	void resetChunkVars();

	/**
	 * Reads flash, and checks if it's erased.
	 */
	bool isErased(uint32_t flashAddress, uint16_t size);
};
