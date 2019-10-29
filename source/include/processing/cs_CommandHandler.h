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

	/** Handle a command without payload, and assuming admin level access.
	 */
	command_result_t handleCommand(const CommandHandlerTypes type, const cmd_source_t source);

	/** Handle a particular command with additional arguments stored in a buffer at a security level.
	 *
	 * The security level is ADMIN by default.
	 */
	command_result_t handleCommand(
			const CommandHandlerTypes type,
			buffer_ptr_t buffer,
			const uint16_t size,
			const cmd_source_t source,
			const EncryptionAccessLevel accessLevel = ADMIN
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

	command_result_t handleCmdNop                   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdGotoDfu               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdReset                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdFactoryReset          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdSetTime               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdIncreaseTx            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdValidateSetup         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdDisconnect            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdResetErrors           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdPwm                   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdSwitch                (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdRelay                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdMultiSwitch           (buffer_ptr_t buffer, const uint16_t size, const cmd_source_t source, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdMeshCommand           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdAllowDimming          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdLockSwitch            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdSetup                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdEnableSwitchcraft     (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdUartMsg               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdUartEnable            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdStateGet              (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdStateSet              (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);

	command_result_t handleCmdSaveBehaviour         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	command_result_t handleCmdReplaceBehaviour		(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);  
	command_result_t handleCmdRemoveBehaviour		(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);  
	command_result_t handleCmdGetBehaviour			(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	

	/**
	 * Reset, after a delay.
	 * TODO: This function doesn't belong in this class.
	 */
	void resetDelayed(uint8_t opCode, uint16_t delayMs=2000);
};

