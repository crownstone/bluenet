#pragma once

#include <structs/buffer/cs_CircularBuffer.h>

// struct __attribute__((packed)) microapp_buffered_mesh_message_t {
// 	stone_id_t stoneId;
// 	uint8_t messageSize;
// 	uint8_t message[MICROAPP_MAX_MESH_MESSAGE_SIZE];
// };

/**
 * The class MicroappCommandHandler has functionality to store a second app (and perhaps in the future even more apps)
 * on another part of the flash memory.
 */
class MicroappCommandHandler {
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappCommandHandler(){};
	MicroappCommandHandler(MicroappCommandHandler const&);
	void operator=(MicroappCommandHandler const&);

	/**
	 * Max number of mesh messages that will be queued.
	 */
	// const uint8_t MICROAPP_MAX_MESH_MESSAGES_BUFFERED = 3;

	/**
	 * Buffer received mesh messages.
	 *
	 * Starts with microapp_buffered_mesh_message_header_t, followed by the message.
	 *
	 * TODO: The buffer should not be stored on the bluenet side.
	 */
	// CircularBuffer<microapp_buffered_mesh_message_t> _meshMessageBuffer;

	/**
	 * Maps interrupts to digital pins.
	 */
	int interruptToDigitalPin(int interrupt);

protected:
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
	 * Handle microapp pin commands for setting pin modes.
	 */
	cs_ret_code_t handleMicroappPinSetModeCommand(microapp_pin_cmd_t* cmd);

	/**
	 * Handle microapp pin commands for reading and writing pins.
	 */
	cs_ret_code_t handleMicroappPinActionCommand(microapp_pin_cmd_t* cmd);

	/**
	 * Handle microapp dimmer and switch commands.
	 */
	cs_ret_code_t handleMicroappDimmerSwitchCommand(microapp_dimmer_switch_cmd_t* cmd);

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

	/**
	 * Handle a received mesh message.
	 *
	 * TODO: We don't want this.
	 */
	// void onMeshMessage(MeshMsgEvent event);

public:
	static MicroappCommandHandler& getInstance() {
		static MicroappCommandHandler instance;
		return instance;
	}

	/**
	 * Handle commands from the microapp
	 */
	cs_ret_code_t handleMicroappCommand(microapp_cmd_t* cmd);

};
