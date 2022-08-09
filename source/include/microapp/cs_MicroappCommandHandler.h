#pragma once

#include <common/cs_Types.h>
#include <cs_MicroappStructs.h>

/**
 * The class MicroappRequestHandler has functionality to store a second app (and perhaps in the future even more apps)
 * on another part of the flash memory.
 */
class MicroappRequestHandler {
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappRequestHandler(){};
	MicroappRequestHandler(MicroappRequestHandler const&);
	void operator=(MicroappRequestHandler const&);

	/**
	 * Maps interrupts to digital pins. See also MicroappController::digitalPinToInterrupt()
	 */
	int interruptToDigitalPin(int interrupt);

protected:

	cs_ret_code_t handleMicroappLogRequest(microapp_sdk_log_t* log);
	cs_ret_code_t handleMicroappPinRequest(microapp_sdk_pin_t* pin);
	cs_ret_code_t handleMicroappSwitchRequest(microapp_sdk_switch_t* switch_);
	cs_ret_code_t handleMicroappServiceDataRequest(microapp_sdk_service_data_t* serviceData);
	cs_ret_code_t handleMicroappTwiRequest(microapp_sdk_twi_t* twi);
	cs_ret_code_t handleMicroappBleRequest(microapp_sdk_ble_t* ble);
	cs_ret_code_t handleMicroappMeshRequest(microapp_sdk_mesh_t* mesh);
	cs_ret_code_t handleMicroappPowerUsageRequest(microapp_sdk_power_usage_t* powerUsage);
	cs_ret_code_t handleMicroappPresenceRequest(microapp_sdk_presence_t* presence);
	cs_ret_code_t handleMicroappControlCommandRequest(microapp_sdk_control_command_t* controlCommand);
	cs_ret_code_t handleMicroappYieldRequest(microapp_sdk_yield_t* yield);

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

public:
	static MicroappRequestHandler& getInstance() {
		static MicroappRequestHandler instance;
		return instance;
	}

	/**
	 * Handle requests from the microapp
	 */
	cs_ret_code_t handleMicroappCommand(microapp_sdk_header_t* header);

};
