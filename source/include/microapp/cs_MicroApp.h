#pragma once

#include <events/cs_EventListener.h>

// Call loop every 10 ticks. The ticks are every 100 ms so this means every second.
//#define MICROAPP_LOOP_FREQUENCY 10

//#define MICROAPP_LOOP_INTERVAL_MS (TICK_INTERVAL_MS * MICROAPP_LOOP_FREQUENCY)

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
		 * For the last chunk we have to do some other things. Validate the entire app, etc.
		 */
		bool _lastChunk;

		/**
		 * Local flag to check if microapp is enabled.
		 */
		bool _enabled;

		/**
		 * Local flag to indicate that ram section has been loaded.
		 */
		bool _loaded;

		/**
		 * Debug mode
		 */
		bool _debug;

	protected:
		/**
		 * Set IPC ram data.
		 */
		void setIpcRam();

		/**
		 * Enable app.
		 */
		uint16_t enableApp(bool flag);

		/**
		 * Check if app is enabled.
		 */
		bool isEnabled();

		/**
		 * Call the loop function
		 */
		void callLoop(int & cntr, int & skip);

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
};
