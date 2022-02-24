#pragma once

#include <cs_MicroappStructs.h>
#include <events/cs_EventListener.h>
#include <structs/buffer/cs_CircularBuffer.h>

extern "C" {
#include <util/cs_DoubleStackCoroutine.h>
}

static_assert(sizeof(bluenet2microapp_ipcdata_t) <= BLUENET_IPC_RAM_DATA_ITEM_SIZE);

/**
 * The IPC buffers can be used to bootstrap communication between microapp and bluenet. However, when in the microapp
 * coroutine context it is convenient if we can immediately yield towards the other context. For this we reserve a bit
 * of space on the stack (apart from stack pointer etc.).
 */
struct coargs_t {
	uintptr_t entry;
	bluenet_io_buffer_t* io_buffer;
};

// Call loop every 10 ticks. The ticks are every 100 ms so this means every second.
#define MICROAPP_LOOP_FREQUENCY 10

#define MICROAPP_LOOP_INTERVAL_MS (TICK_INTERVAL_MS * MICROAPP_LOOP_FREQUENCY)

// The number of 8 interrupt service routines per type should be sufficient.
const uint8_t MAX_PIN_ISR_COUNT = 8;
const uint8_t MAX_BLE_ISR_COUNT = 8;

struct pin_isr_t {
	uint8_t pin;
	bool registered;
};

struct ble_isr_t {
	MicroappBleEventType type;
	uint8_t id;
	bool registered;
};

struct __attribute__((packed)) microapp_buffered_mesh_message_t {
	stone_id_t stoneId;  // Stone ID of the sender.
	uint8_t messageSize;
	uint8_t message[MICROAPP_MAX_MESH_MESSAGE_SIZE];
};

/**
 * The class MicroappProtocol has functionality to store a second app (and perhaps in the future even more apps) on
 * another part of the flash memory.
 */
class MicroappProtocol : public EventListener {
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappProtocol();
	MicroappProtocol(MicroappProtocol const&);
	void operator=(MicroappProtocol const&);

	/**
	 * Addressees of pin interrupt service routines.
	 */
	pin_isr_t _pinIsr[MAX_PIN_ISR_COUNT];

	/**
	 * Addressees of ble interrupt service routines.
	 */
	ble_isr_t _bleIsr[MAX_BLE_ISR_COUNT];

	/**
	 * Coroutine for microapp.
	 */
	coroutine_t _coroutine;

	/**
	 * The maximum number of consecutive calls to a microapp.
	 */
	const uint8_t MAX_CONSECUTIVE_MESSAGES = 8;

	/**
	 * Call counter
	 */
	uint8_t _callCounter;

	/**
	 * Store data for callbacks towards the microapp.
	 */
	CircularBuffer<uint8_t>* _callbackData;

	/**
	 * Flag to indicate whether to forward scanned devices to microapp
	 */
	bool _microappIsScanning;

	/**
	 * Max number of mesh messages that will be queued.
	 */
	const uint8_t MAX_MESH_MESSAGES_BUFFERED = 3;

	/**
	 * Buffer received mesh messages.
	 *
	 * Starts with microapp_buffered_mesh_message_header_t, followed by the message.
	 */
	CircularBuffer<microapp_buffered_mesh_message_t> _meshMessageBuffer;

	/**
	 * Counter that counts how many callbacks have been received.
	 */
	uint32_t _callbackReceivedCounter;

	/**
	 * Counter that counts how many callbacks have been executed until the end.
	 */
	uint32_t _callbackEndCounter;

	/**
	 * Counter that counts how many callbacks have been executed until the end.
	 */
	uint32_t _callbackFailCounter;

	/**
	 * Callback execute counter
	 */
	int8_t _callbackExecCounter;

	/**
	 * Limit the number of callbacks in a tick (if -1) there is no limit.
	 */
	const int8_t MAX_CALLBACKS_WITHIN_A_TICK = 10;

	/**
	 * Maximum number of soft interrupts in parallel.
	 */
	const uint8_t MAX_SOFTINTERRUPTS_IN_PARALLEL = 2;

	/**
	 * To throttle the ticks themselves.
	 */
	uint8_t _tickCounter;

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
	void writeCallback();

	/**
	 * Register GPIO pin.
	 */
	bool registerGpio(uint8_t pin);

	/**
	 * Register BLE callback.
	 */
	bool registerBleCallback(uint8_t id);

	/**
	 * Callback for GPIO events.
	 */
	void performCallbackGpio(uint8_t pin);

	/**
	 * Callback BLE.
	 */
	void performCallbackIncomingBLEAdvertisement(scanned_device_t* dev);

	/**
	 * A callback already in progress.
	 */
	bool callbackInProgress();

	/**
	 * Initialize memory for the microapp.
	 */
	uint16_t initMemory();

	/**
	 * Load ram information, set by microapp.
	 */
	uint16_t interpretRamdata();

	/**
	 * Handle a received mesh message.
	 */
	void onMeshMessage(MeshMsgEvent event);

	/**
	 * Handle a scanned BLE device.
	 */
	void onDeviceScanned(scanned_device_t* dev);

	/**
	 * After particular microapp commands we want to stop the microapp (end of loop etc.) and continue with bluenet.
	 * This function returns true for such commands.
	 *
	 * @param[in] cmd                            Any microapp command
	 */
	bool stopAfterMicroappCommand(microapp_cmd_t* cmd);

	/**
	 * Handle microapp log commands.
	 *
	 * @param[in] cmd                            A microapp command for logging to serial.
	 */
	cs_ret_code_t handleMicroappLogCommand(microapp_log_cmd_t* cmd);

	/**
	 * Handle microapp delay commands.
	 *
	 * @param[in] cmd                            A microapp command to introduce a delay for the microapp itself.
	 */
	cs_ret_code_t handleMicroappDelayCommand(microapp_delay_cmd_t* cmd);

	/**
	 * Handle microapp pin commands.
	 */
	cs_ret_code_t handleMicroappPinCommand(microapp_pin_cmd_t* cmd);

	/**
	 * Handle microapp pin switching commands.
	 */
	cs_ret_code_t handleMicroappPinSwitchCommand(microapp_pin_cmd_t* cmd);

	/**
	 * Handle microapp pin commands for setting pin modes.
	 */
	cs_ret_code_t handleMicroappPinSetModeCommand(microapp_pin_cmd_t* cmd);

	/**
	 * Handle microapp pin commands for reading and writing pins.
	 */
	cs_ret_code_t handleMicroappPinActionCommand(microapp_pin_cmd_t* cmd);

	/**
	 * Handle microapp commands for advertising microapp state in service data.
	 */
	cs_ret_code_t handleMicroappServiceDataCommand(microapp_service_data_cmd_t* cmd);

	/**
	 * Handle microapp TWI commands.
	 */
	cs_ret_code_t handleMicroappTwiCommand(microapp_twi_cmd_t* cmd);

	/**
	 * Handle microapp BLE commands.
	 */
	cs_ret_code_t handleMicroappBleCommand(microapp_ble_cmd_t* cmd);

	/**
	 * Handle microapp commands for power usage requests.
	 */
	cs_ret_code_t handleMicroappPowerUsageCommand(microapp_power_usage_cmd_t* cmd);

	/**
	 * Handle microapp commands for presence requests.
	 */
	cs_ret_code_t handleMicroappPresenceCommand(microapp_presence_cmd_t* cmd);

	/**
	 * Handle microapp commands for sending and reading mesh messages.
	 */
	cs_ret_code_t handleMicroappMeshCommand(microapp_mesh_cmd_t* cmd);

public:
	static MicroappProtocol& getInstance() {
		static MicroappProtocol instance;
		return instance;
	}

	/**
	 * Set IPC ram data.
	 */
	void setIpcRam();

	/**
	 * Actually run the app.
	 */
	void callApp(uint8_t appIndex);

	/**
	 * Tick microapp
	 */
	void tickMicroapp(uint8_t appIndex);

	/**
	 * Receive events (for example for i2c)
	 */
	void handleEvent(event_t& event);

	/**
	 * Handle commands from the microapp
	 */
	cs_ret_code_t handleMicroappCommand(microapp_cmd_t* cmd);
};
