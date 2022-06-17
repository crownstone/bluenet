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
 * Interrupt service routines for pins as registered by the microapp.
 */
struct microapp_pin_isr_t {
	uint8_t pin;
	bool registered;
};

/**
 * Interrupt service routines for bluetooth events as registered by the microapp.
 */
struct microapp_ble_isr_t {
	MicroappBleEventType type;
	uint8_t id;
	bool registered;
};

/**
 * Interrupt service routines for received mesh events as registered by the microapp.
 */
struct microapp_mesh_isr_t {
	uint8_t id;
	bool registered;
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
	 * Limit maximum number of to be registered service routines for the microapp.
	 */
	static const uint8_t MICROAPP_MAX_PIN_ISR_COUNT = 8;

	/**
	 * Limit maximum number of to be registered service routines for the microapp.
	 */
	static const uint8_t MICROAPP_MAX_BLE_ISR_COUNT = 4;

	/**
	 * Limit the number of callbacks in a tick (if -1) there is no limit.
	 */
	const int8_t MAX_CALLBACKS_WITHIN_A_TICK = 10;

	/**
	 * The maximum number of consecutive calls to a microapp.
	 */
	const uint8_t MICROAPP_MAX_NUMBER_CONSECUTIVE_MESSAGES = 8;

	/**
	 * Addressees of pin interrupt service routines.
	 */
	microapp_pin_isr_t _pinIsr[MICROAPP_MAX_PIN_ISR_COUNT];

	/**
	 * Addressees of ble interrupt service routines.
	 */
	microapp_ble_isr_t _bleIsr[MICROAPP_MAX_BLE_ISR_COUNT];

	/**
	 * Addressee of mesh interrupt service routines.
	 */
	microapp_mesh_isr_t _meshIsr;

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
	 * Call counter
	 */
	uint8_t _callCounter;

	/**
	 * Soft interrupt counter
	 */
	int8_t _softInterruptCounter;

	/**
	 * To throttle the ticks themselves.
	 */
	uint8_t _tickCounter;

	/**
	 * Flag to indicate whether to forward scanned devices to microapp.
	 */
	bool _microappIsScanning;

	/**
	 * Keeps track of how many empty soft interrupt slots are available on the microapp side
	 */
	uint8_t _emptySoftInterrupts = 1;

	/**
	 * Maps digital pins to interrupts.
	 */
	int digitalPinToInterrupt(int pin);

	/**
	 * Checks whether the microapp has empty interrupt slots to deal with a new softInterrupt
	 */
	bool softInterruptInProgress();

protected:
	/**
	 * Call the loop function (internally).
	 */
	void callMicroapp();

	/**
	 * Get the command from the microapp.
	 */
	bool retrieveCommand();

	/**
	 * Write the callback.
	 */
	void softInterrupt();

	/**
	 * Check if start address of the microapp is within the flash boundaries assigned to the microapps.
	 */
	cs_ret_code_t checkFlashBoundaries(uint8_t appIndex, uintptr_t address);

	/**
	 * Register GPIO pin.
	 */
	bool registerSoftInterruptSlotGpio(uint8_t pin);

	/**
	 * Interrupt microapp with GPIO event.
	 */
	void softInterruptGpio(uint8_t pin);

	/**
	 * Interrupt microapp with new BLE event.
	 */
	void softInterruptBle(scanned_device_t* dev);

	/**
	 * Interrupt microapp with new Mesh event
	 */
	void softInterruptMesh(MeshMsgEvent* event);

	/**
	 * Get incoming microapp buffer (from coargs).
	 */
	uint8_t* getInputMicroappBuffer();

	/**
	 * Get outgoing microapp buffer (from coargs).
	 */
	uint8_t* getOutputMicroappBuffer();

	/**
	 * Handle a scanned BLE device.
	 */
	void onDeviceScanned(scanned_device_t* dev);

	void onReceivedMeshMessage(MeshMsgEvent* event);

	/**
	 * After particular microapp commands we want to stop the microapp (end of loop etc.) and continue with bluenet.
	 * This function returns true for such commands.
	 *
	 * @param[in] cmd                            Any microapp command
	 */
	bool stopAfterMicroappCommand(microapp_cmd_t* cmd);

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
	 * Shortcut setScanning for MicroappCommandHandler to update state of this instance.
	 */
	void setScanning(bool scanning);

	/**
	 * Set the number of empty softInterrupt slots
	 *
	 * @param emptySoftInterrupts The new value
	 */
	void setEmptySoftInterrupts(uint8_t emptySoftInterrupts);

	/**
	 * Increment the number of empty softInterrupt slots
	 * (can be called when a softInterrupt returns)
	 */
	void incrementEmptySoftInterrupts();

	/**
	 * Register slot for BLE soft interrupt.
	 */
	bool registerSoftInterruptSlotBle(uint8_t id);

	/**
	 * Register slot for Mesh soft interrupt.
	 */
	bool registerSoftInterruptSlotMesh(uint8_t id);

	/**
	 * Receive events (for example for i2c)
	 */
	void handleEvent(event_t& event);
};
