#pragma once

#include <events/cs_EventListener.h>

extern "C" {
#include <util/cs_DoubleStackCoroutine.h>
}

typedef struct {
	coroutine *c;
	int cntr;
	int delay;
} coargs;

// Call loop every 10 ticks. The ticks are every 100 ms so this means every second.
#define MICROAPP_LOOP_FREQUENCY 10

#define MICROAPP_LOOP_INTERVAL_MS (TICK_INTERVAL_MS * MICROAPP_LOOP_FREQUENCY)

/**
 * The class MicroAppProtocol has functionality to store a second app (and perhaps in the future even more apps) on another
 * part of the flash memory.
 */
class MicroAppProtocol { //: public EventListener {
	private:
		/**
		 * Singleton, constructor, also copy constructor, is private.
		 */
		MicroAppProtocol();
		MicroAppProtocol(MicroAppProtocol const&);
		void operator=(MicroAppProtocol const &);

		/**
		 * Local flag to check if app did boot. This flag is set to true after the main of the microapp is called
		 * and will be set after that function returns. If there is something wrong with the microapp it can be used
		 * to disable the microapp.
		 *
		 * TODO: Of course, if there is something wrong, _booted will not be reached. The foolproof implementation
		 * sets first the state to a temporarily "POTENTIAL BOOT FAILURE" and if it after a reboots finds the state
		 * at this particular step, it will disable the app rather than try again. The app has to be then enabled
		 * explicitly again.
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

		/**
		 * Coroutine for microapp.
		 */
		coroutine _coroutine;

		/**
		 * Arguments to the coroutine.
		 */
		coargs _coargs;

		/**
		 * A counter used for the coroutine (to e.g. set the number of ticks for "delay" functionality).
		 */
		int _cocounter;

		/**
		 * Implementing count down for a coroutine counter.
		 */
		int _coskip;

	protected:

		/**
		 * Call the loop function (internally).
		 */
		void callLoop(int & cntr, int & skip);

		/**
		 * Initialize memory for the microapp.
		 */
		uint16_t initMemory();

		/**
		 * Load ram information, set by microapp.
		 */
		uint16_t interpretRamdata();
	public:
		static MicroAppProtocol& getInstance() {
			static MicroAppProtocol instance;
			return instance;
		}

		/**
		 * Initialize fstorage. Allocate buffer.
		 */
		uint16_t init();

		/**
		 * Set IPC ram data.
		 */
		void setIpcRam();

		/**
		 * Actually run the app.
		 */
		void callApp();

		/**
		 * Call setup and loop functions.
		 */
		void callSetupAndLoop();

};
