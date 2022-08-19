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
	/**
	 * SDK-type specific handlers. Called from handleMicroappRequest.
	 * The incoming header ack always needs to be set in these handlers.
	 * The microapp may check the result of the request handling via this ack field
	 */
	cs_ret_code_t handleMicroappLogRequest(microapp_sdk_log_header_t* log);
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

public:
	static MicroappRequestHandler& getInstance() {
		static MicroappRequestHandler instance;
		return instance;
	}

	/**
	 * Handle requests from the microapp.
	 * Gets incoming buffer, checks SDK type in the header and calls the appropriate handler
	 */
	cs_ret_code_t handleMicroappRequest(microapp_sdk_header_t* header);
};
