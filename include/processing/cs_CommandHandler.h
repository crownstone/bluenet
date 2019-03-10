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

/** Commands to be executed at a later stage.
 *
 * A command can be exected later. Arguments are stored in the delayed_command_t struct within a buffer of variable
 * size.
 */
struct delayed_command_t {
	//! Type of command
	CommandHandlerTypes type;
	//! Size of buffer (can be 0)
	uint16_t size;
	//! Pointer to buffer
	buffer_ptr_t buffer;
};

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
	cs_ret_code_t handleCommand(const CommandHandlerTypes type);

	/** Handle a particular command with additional arguments stored in a buffer at a security level.
	 *
	 * The security level is ADMIN by default.
	 */
	cs_ret_code_t handleCommand(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size,
			const EncryptionAccessLevel accessLevel = ADMIN);

	/** Handle a particular command, but with a specified delay.
	 */
	cs_ret_code_t handleCommandDelayed(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size,
			const uint32_t delay);

	/** Reset, after a delay (2 seconds).
	 *
	 * There is a DFU reset or a GPREGRET reset.
	 */
	void resetDelayed(uint8_t opCode, uint16_t delayMs=2000);

	// Handle events as EventListener
	void handleEvent(event_t & event);

private:

	CommandHandler();

	app_timer_t      _delayTimerData;
	app_timer_id_t   _delayTimerId;
	app_timer_t      _resetTimerData;
	app_timer_id_t   _resetTimerId;

	const boards_config_t* _boardConfig;

	EncryptionAccessLevel getRequiredAccessLevel(const CommandHandlerTypes type);
	bool allowedAsMeshCommand(const CommandHandlerTypes type);

	cs_ret_code_t handleCmdNop                   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdGotoDfu               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdReset                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdEnableMesh            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdEnableEncryption      (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdEnableIbeacon         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdEnableScanner         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdRequestServiceData    (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdFactoryReset          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdSetTime               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdScheduleEntrySet      (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdScheduleEntryClear    (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdIncreaseTx            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdValidateSetup         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdKeepAlive             (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdKeepAliveState        (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdKeepAliveRepeatLast   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdKeepAliveMesh         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdUserFeedBack          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdDisconnect            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdSetLed                (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdResetErrors           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdPwm                   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdSwitch                (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdRelay                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdMultiSwitch           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdMeshCommand           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdEnableContPowerMeasure(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdAllowDimming          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdLockSwitch            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdSetup                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdEnableSwitchcraft     (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdUartMsg               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	cs_ret_code_t handleCmdUartEnable            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);

};

