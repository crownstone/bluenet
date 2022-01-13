#pragma once

#include <cs_MicroappStructs.h>
#include <events/cs_EventListener.h>
#include <structs/buffer/cs_CircularBuffer.h>

extern "C" {
#include <util/cs_DoubleStackCoroutine.h>
}

static_assert (sizeof(microapp2bluenet_ipcdata_t) <= BLUENET_IPC_RAM_DATA_ITEM_SIZE);
static_assert (sizeof(bluenet2microapp_ipcdata_t) <= BLUENET_IPC_RAM_DATA_ITEM_SIZE);

struct coargs_payload_t {
	uint8_t *data;
	uint16_t size;
};

struct coargs_t {
	coroutine_t* coroutine;
	uintptr_t entry;
	coargs_payload_t microapp2bluenet;
	coargs_payload_t bluenet2microapp;
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
	 * A counter used for the coroutine (to e.g. set the number of ticks for "delay" functionality).
	 */
	int _cocounter;

	/**
	 * Implementing count down for a coroutine counter.
	 */
	int _coskip;

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
	void performCallbackMeshMessage(scanned_device_t* dev);

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
	 * Flag as being processed by bluenet. This will overwrite the cmd field so that a command is not performed twice.
	 *
	 * @param[in] cmd                            Any microapp command
	 */
	void setAsProcessed(microapp_cmd_t* cmd);

	/**
	 * After particular microapp commands we want to stop the microapp (end of loop etc.) and continue with bluenet.
	 * This function returns true for such commands.
	 */
	bool stopAfterMicroappCommand(uint8_t* payload, uint16_t length);

	/**
	 * Handle microapp log commands.
	 */
	cs_ret_code_t handleMicroappLogCommand(uint8_t* payload, uint16_t length);

	/**
	 * Handle microapp delay commands.
	 */
	cs_ret_code_t handleMicroappDelayCommand(microapp_delay_cmd_t* delay_cmd);

	/**
	 * Handle microapp pin commands.
	 */
	cs_ret_code_t handleMicroappPinCommand(microapp_pin_cmd_t* pin_cmd);

	/**
	 * Handle microapp pin switching commands.
	 */
	cs_ret_code_t handleMicroappPinSwitchCommand(microapp_pin_cmd_t* pin_cmd);

	/**
	 * Handle microapp pin commands for setting pin modes.
	 */
	cs_ret_code_t handleMicroappPinSetModeCommand(microapp_pin_cmd_t* pin_cmd);

	/**
	 * Handle microapp pin commands for reading and writing pins.
	 */
	cs_ret_code_t handleMicroappPinActionCommand(microapp_pin_cmd_t* pin_cmd);

	/**
	 * Handle microapp commands for advertising microapp state in service data.
	 */
	cs_ret_code_t handleMicroappServiceDataCommand(uint8_t* payload, uint16_t length);

	/**
	 * Handle microapp TWI commands.
	 */
	cs_ret_code_t handleMicroappTwiCommand(microapp_twi_cmd_t* twi_cmd);

	/**
	 * Handle microapp BLE commands.
	 */
	cs_ret_code_t handleMicroappBleCommand(microapp_ble_cmd_t* ble_cmd);

	/**
	 * Handle microapp commands for power usage requests.
	 */
	cs_ret_code_t handleMicroappPowerUsageCommand(uint8_t* payload, uint16_t length);

	/**
	 * Handle microapp commands for presence requests.
	 */
	cs_ret_code_t handleMicroappPresenceCommand(uint8_t* payload, uint16_t length);

	/**
	 * Handle microapp commands for sending and reading mesh messages.
	 */
	cs_ret_code_t handleMicroappMeshCommand(microapp_mesh_header_t* meshCommand, uint8_t* payload, size_t payloadSize);

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
	cs_ret_code_t handleMicroappCommand(uint8_t* payload, uint16_t length);
};
