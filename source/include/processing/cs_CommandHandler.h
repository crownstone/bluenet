/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_Stack.h>
#include <common/cs_Types.h>
#include <protocol/cs_CommandTypes.h>


/**
 * Every command goes through the CommandHandler.
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
	 * Handle a command, see handleCommand()
	 */
	command_result_t handleCommand(const CommandHandlerTypes type, const cmd_source_t source);

	/**
	 * Handle a a command.
	 *
	 * @param[in]  type           Type of command.
	 * @param[in]  commandData    Data of the command.
	 * @param[in]  source         Source of the entity that issuce this command.
	 * @param[in]  accessLevel    Access level of the entity that issued this command. Default is ADMIN.
	 * @param[out] resultData     Buffer (that can be NULL) to put the result data in. Default is NULL.
	 * @return                    Result of the command.
	 */
	command_result_t handleCommand(
			const CommandHandlerTypes type,
			cs_data_t commandData,
			const cmd_source_t source,
			const EncryptionAccessLevel accessLevel = ADMIN,
			cs_data_t resultData = cs_data_t()
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

	command_result_t handleCmdNop                   (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdGotoDfu               (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdReset                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdFactoryReset          (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdSetTime               (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdSetSunTime            (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdIncreaseTx            (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdValidateSetup         (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdDisconnect            (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdResetErrors           (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdPwm                   (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdSwitch                (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdRelay                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdMultiSwitch           (cs_data_t commandData, const cmd_source_t source, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdMeshCommand           (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdAllowDimming          (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdLockSwitch            (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdSetup                 (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdUartMsg               (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdStateGet              (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	command_result_t handleCmdStateSet              (cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData);
	
	// delegate a command in the form of an event of given [typ], passing the commandData and using the allocated [resultData]
	command_result_t dispatchEventForCommand(CS_TYPE typ, cs_data_t commandData, cs_data_t resultData);

	/**
	 * Reset, after a delay.
	 * TODO: This function doesn't belong in this class.
	 */
	void resetDelayed(uint8_t opCode, uint16_t delayMs=2000);
};

