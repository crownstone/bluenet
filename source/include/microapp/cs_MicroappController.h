#pragma once

#include <cs_MicroappStructs.h>
#include <events/cs_EventListener.h>

extern "C" {
#include <util/cs_DoubleStackCoroutine.h>
}

static_assert(sizeof(bluenet2microapp_ipcdata_t) <= BLUENET_IPC_RAM_DATA_ITEM_SIZE);

/**
 * The IPC buffers can be used to bootstrap communication between microapp and bluenet. However, when in the microapp
 * coroutine context it is convenient if we can immediately yield towards the other context. For this we reserve a bit
 * of space on the stack (apart from stack pointer etc.).
 */
struct coroutine_args_t {
	uintptr_t entry;
	bluenet_io_buffer_t* io_buffer;
};

/**
 * @struct microapp_interrupt_registration_t
 * Struct for keeping track of registered interrupts from the microapp
 */
struct microapp_interrupt_registration_t {
	bool registered = false;
	uint8_t major = 0;
	uint8_t minor = 0;
};

/**
 * The class MicroappController has functionality to store a second app (and perhaps in the future even more apps) on
 * another part of the flash memory.
 */
class MicroappController : public EventListener {
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappController();
	MicroappController(MicroappController const&);
	void operator=(MicroappController const&);

	/**
	 * Limit the number of interrupts in a tick (if -1) there is no limit.
	 */
	const int8_t MICROAPP_MAX_INTERRUPTS_WITHIN_A_TICK = 10;

	/**
	 * The maximum number of consecutive calls to a microapp.
	 */
	const uint8_t MICROAPP_MAX_NUMBER_CONSECUTIVE_CALLS = 8;

	/**
	 * The maximum number of registered interrupts
	 */
	static const uint8_t MICROAPP_MAX_INTERRUPT_REGISTRATIONS = 10;

	/**
	 * Buffer for keeping track of registered interrupts
	 */
	microapp_interrupt_registration_t _interruptRegistrations[MICROAPP_MAX_INTERRUPT_REGISTRATIONS];

	/**
	 * Coroutine for microapp.
	 */
	coroutine_t _coroutine;

	/*
	 * Shared state to both the microapp and the bluenet code. This is used as an argument to the coroutine. It can
	 * later be used to get information back and forth between microapp and bluenet.
	 */
	coroutine_args_t sharedState;

	/**
	 * Counter for interrupts within a tick
	 */
	int8_t _interruptCounter;

	/**
	 * Counter for consecutive microapp calls
	 */
	uint8_t _consecutiveMicroappCallCounter;

	/**
	 * To throttle the ticks themselves.
	 */
	uint8_t _tickCounter;

	/**
	 * Flag to indicate whether to forward scanned devices to microapp.
	 */
	bool _microappIsScanning;

	/**
	 * Keeps track of how many empty interrupt slots are available on the microapp side
	 */
	uint8_t _emptyInterruptSlots = 1;

	/**
	 * Maps digital pins to interrupts. See also MicroappCommandHandler::interruptToDigitalPin()
	 */
	int digitalPinToInterrupt(int pin);

	/**
	 * Checks whether an interrupt registration already exists
	 */
	bool interruptRegistered(uint8_t major, uint8_t minor);

	/**
	 * Checks whether the microapp has empty interrupt slots to deal with a new softInterrupt
	 */
	bool allowInterrupts();

protected:
	/**
	 * Call the loop function (internally).
	 */
	void callMicroapp();

	/**
	 * Read the ack from the microapp and returns whether to handle its request
	 */
	bool handleAck();

	/**
	 * Get the request from the microapp and let the request handler handle it
	 */
	bool handleRequest();

	/**
	 * Call the microapp in an interrupt context
	 */
	void generateInterrupt();

	/**
	 * Check if start address of the microapp is within the flash boundaries assigned to the microapps.
	 */
	cs_ret_code_t checkFlashBoundaries(uint8_t appIndex, uintptr_t address);

	/**
	 * Get incoming microapp buffer (from coargs).
	 */
	uint8_t* getInputMicroappBuffer();

	/**
	 * Get outgoing microapp buffer (from coargs).
	 */
	uint8_t* getOutputMicroappBuffer();

	/**
	 * Handle a GPIO event
	 */
	void onGpioUpdate(uint8_t pinIndex);

	/**
	 * Handle a scanned BLE device.
	 */
	void onDeviceScanned(scanned_device_t* dev);

	/**
	 * Handle a received mesh message and determine whether to forward it to the microapp.
	 *
	 * @param event the EVT_RECV_MESH_MSG event data
	 */
	void onReceivedMeshMessage(MeshMsgEvent* event);

	/**
	 * After particular microapp requests we want to stop the microapp (end of loop etc.) and continue with bluenet.
	 * This function returns true for such requests.
	 *
	 * @param[in] header        Header of the microapp request
	 */
	bool stopAfterMicroappRequest(microapp_sdk_header_t* header);

public:
	static MicroappController& getInstance() {
		static MicroappController instance;
		return instance;
	}

	/**
	 * Set IPC ram data.
	 */
	void setIpcRam();

	/**
	 * Initialize memory.
	 */
	uint16_t initMemory(uint8_t appIndex);

	/**
	 * Actually run the app.
	 */
	void callApp(uint8_t appIndex);

	/**
	 * Tick microapp
	 */
	void tickMicroapp(uint8_t appIndex);

	/**
	 * Register interrupts that allow generation of interrupts to the microapp
	 */
	cs_ret_code_t registerInterrupt(uint8_t major, uint8_t minor);

	/**
	 * Set the number of empty interrupt slots
	 *
	 * @param emptyInterruptSlots The new value
	 */
	void setEmptyInterruptSlots(uint8_t emptyInterruptSlots);

	/**
	 * Increment the number of empty interrupt slots
	 */
	void incrementEmptyInterruptSlots();

	/**
	 * Enable or disable BLE scanned device interrupt calls
	 */
	void setScanning(bool scanning);

	/**
	 * Receive events
	 */
	void handleEvent(event_t& event);
};
