#pragma once

#include <events/cs_EventListener.h>

/**
 * The class MicroApp has functionality to store a second app (and perhaps in the future even more apps) on another
 * part of the flash memory.
 */
class MicroApp: public EventListener {
	private:
		/**
		 * Singleton, constructor, also copy constructor, is private.
		 */
		MicroApp();
		MicroApp(MicroApp const&);
		void operator=(MicroApp const &);

		/**
		 * The last message we have sent. It has a repeat value that will start at "number_of_notifications". If it
		 * is at zero, it will not be repeated anymore.
		 */
		TYPIFY(EVT_MICROAPP) _prevMessage;

		/**
		 * We will only allocate the buffer on init(). The buffer is required because when writing to flash, a stack
		 * allocated pointer might already be gone. The buffer will be of size MICROAPP_CHUNK_SIZE.
		 */
		uint8_t *_buffer;

		/**
		 * For the last chunk we have to do some other things. Validate the entire app, etc.
		 */
		bool _lastChunk;

		/**
		 * Local flag to check if microapp is enabled.
		 */
		bool _enabled;

		/**
		 * Local flag to check if app did boot.
		 */
		bool _booted;

		/**
		 * Local flag to indicate that ram section has been loaded.
		 */
		bool _loaded;

		/**
		 * Debug mode
		 */
		bool _debug;

		/**
		 * Address to setup() function.
		 */
		uintptr_t _setup;

		/**
		 * Address to loop() function.
		 */
		uintptr_t _loop;

		static const int number_of_notifications = 3;

	protected:
		/**
		 * Write a chunk to flash memory. Writes to buffer at index start + index * CHUNK_SIZE up to size of the data.
		 */
		uint16_t writeChunk(uint8_t index, const uint8_t *data, uint16_t size);

		/**
		 * Validate chunk by comparing its calculated checksum with the checksum in the packet.
		 */
		uint16_t validateChunk(const uint8_t * const data, uint16_t size, uint16_t compare);

		/**
		 * Check if app will fit in the reserved pages.
		 */
		uint16_t checkAppSize(uint16_t size);

		/**
		 * Erases all pages of the MicroApp binary.
		 */
		uint16_t erasePages();

		/**
		 * Initialize memory for the microapp.
		 */
		uint16_t initMemory();

		/**
		 * Set IPC ram data.
		 */
		void setIpcRam();

		/**
		 * Store in flash information about the app (start address, checksum, etc.)
		 */
		void storeAppMetadata(uint8_t id, uint16_t checksum, uint16_t size);

		/**
		 * Set app to be valid (only the FDS record field).
		 */
		void setAppValid();

		/**
		 * Enable app.
		 */
		uint16_t enableApp(bool flag);

		/**
		 * Check if app is enabled.
		 */
		bool isEnabled();

		/**
		 * Actually run the app.
		 */
		void callApp();

		/**
		 * Load ram information, set by microapp.
		 */
		uint16_t interpretRamdata();
	public:
		static MicroApp& getInstance() {
			static MicroApp instance;
			return instance;
		}

		/**
		 * Initialize fstorage. Allocate buffer.
		 */
		uint16_t init();

		/**
		 * The tick function is used to send more notifications than one.
		 */
		void tick();

		/**
		 * Handle incoming events.
		 */
		void handleEvent(event_t & event);

		/**
		 * Handle incoming packet. Returns err_code.
		 */
		uint32_t handlePacket(microapp_packet_header_t *packet_stub);

		/**
		 * When fstorage is done, this function will be called (indirectly through app_scheduler).
		 */
		void handleFileStorageEvent(nrf_fstorage_evt_t *evt);

		/**
		 * Check if app is valid (only checking the FDS record field).
		 */
		bool isAppValid();

		/**
		 * Validate the overall binary. This goes through flash and checks it completely. All flash write operations
		 * have to have finished before.
		 */
		uint16_t validateApp();


};
