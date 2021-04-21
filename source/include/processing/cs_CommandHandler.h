/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_Stack.h>
#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>
#include <protocol/cs_CommandTypes.h>


/**
 * Every command from an external device such as a smartphone goes through the CommandHandler.
 *
 * # Handlers
 *
 * To implement a new command:
 *   - Add a new type to <cs_Types.h>
 *   - Define a custom handler for this type in this class.
 *   - Implement a function to handle this type.
 *     - The commandData contains a pointer to a data buffer.
 *     - This data will be gone after function returns.
 *     - In most functions there is no explicit memcpy, but a struct assignment (including member arrays).
 *     - When sending the data through as event, a pointer to the struct can be used.
 *
 * # Access Level
 *
 * TODO: In handleCommand is every function by default executed at ADMIN level. This should be at the lowest privilege
 * level.
 */
class CommandHandler : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static CommandHandler& getInstance() {
		static CommandHandler instance;
		return instance;
	}

	/** Initialize command handler given board configuration.
	 */
	void init(const boards_config_t* board);

	/**
	 * Handle a a command.
	 *
	 * @param[in]  protocolVersion     Protocol version of the command.
	 * @param[in]  type                Type of command.
	 * @param[in]  commandData         Data of the command.
	 * @param[in]  source              Source of the entity that issuce this command.
	 * @param[in]  accessLevel         Access level of the entity that issued this command. Default is ADMIN.
	 * @param[out] resultData          Buffer (that can be NULL) to put the result data in. Default is NULL.
	 * @return                         Result of the command.
	 */
	void handleCommand(
			uint8_t protocolVersion,
			const CommandHandlerTypes type,
			cs_data_t commandData,
			const cmd_source_with_counter_t source,
			const EncryptionAccessLevel accessLevel,
			cs_result_t & result
			);

	// Handle events as EventListener
	void handleEvent(event_t & event);

private:

	CommandHandler();

	app_timer_t      _resetTimerData;
	app_timer_id_t   _resetTimerId;

	const boards_config_t* _boardConfig;

	EncryptionAccessLevel getRequiredAccessLevel(const CommandHandlerTypes type);
	bool allowedAsMeshCommand(const CommandHandlerTypes type);

	void handleCmdNop                     (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGotoDfu                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetBootloaderVersion    (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetUicrData             (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdReset                   (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdFactoryReset            (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetMacAddress           (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetHardwareVersion      (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetFirmwareVersion      (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdSetSunTime              (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetTime                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdIncreaseTx              (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdValidateSetup           (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdDisconnect              (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdResetErrors             (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdPwm                     (cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdSwitch                  (cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdRelay                   (cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdMultiSwitch             (cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdMeshCommand             (uint8_t protocol, cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accesss_resulLevel, cs_result_t & result);
	void handleCmdAllowDimming            (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdLockSwitch              (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdSetup                   (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdUartMsg                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdHubData                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdStateGet                (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdStateSet                (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdRegisterTrackedDevice   (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdTrackedDeviceHeartbeat  (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdGetUptime               (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	void handleCmdMicroappUpload          (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result);
	
	/**
	 * Delegate a command via an event.
	 */
	void dispatchEventForCommand(CS_TYPE type, cs_data_t commandData, const cmd_source_with_counter_t& source, cs_result_t & result);

	/**
	 * Reset, after a delay.
	 * TODO: This function doesn't belong in this class.
	 */
	void resetDelayed(uint8_t opCode, uint16_t delayMs=2000);
};

