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

	/** Handle a particular command directly.
	 *
	 * Examples of the types of commands that can be given to the CommandHandler: CMD_NOP, CMD_GOTO_DFU, CMD_RESET,
	 * CMD_ENABLE_MESH, CMD_ENABLE_ENCRYPTION, CMD_ENABLE_IBEACON, CMD_ENABLE_SCANNER, CMD_SCAN_DEVICES,
	 * CMD_REQUEST_SERVICE_DATA, and many more.
	 */
	ERR_CODE handleCommand(const CommandHandlerTypes type);

	/** Handle a particular command with additional arguments stored in a buffer at a security level.
	 *
	 * The security level is ADMIN by default.
	 */
	ERR_CODE handleCommand(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size, 
			const EncryptionAccessLevel accessLevel = ADMIN);

	/** Handle a particular command, but with a specified delay.
	 */
	ERR_CODE handleCommandDelayed(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size, 
			const uint32_t delay);

	/** Reset, after a delay (2 seconds). 
	 *
	 * There is a DFU reset or a GPREGRET reset.
	 */
	void resetDelayed(uint8_t opCode, uint16_t delayMs=2000);

	// Handle events as EventListener
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

private:

	CommandHandler();

	app_timer_t      _delayTimerData;
	app_timer_id_t   _delayTimerId;
	app_timer_t      _resetTimerData;
	app_timer_id_t   _resetTimerId;

	const boards_config_t* _boardConfig;

	EncryptionAccessLevel getRequiredAccessLevel(const CommandHandlerTypes type);
	bool allowedAsMeshCommand(const CommandHandlerTypes type);

	ERR_CODE handleCmdNop                   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdGotoDfu               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdReset                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdEnableMesh            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdEnableEncryption      (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdEnableIbeacon         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdEnableScanner         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdScanDevices           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdRequestServiceData    (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdFactoryReset          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdSetTime               (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdScheduleEntrySet      (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdScheduleEntryClear    (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdIncreaseTx            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdValidateSetup         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdKeepAlive             (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdKeepAliveState        (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdKeepAliveRepeatLast   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdKeepAliveMesh         (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdUserFeedBack          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdDisconnect            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdSetLed                (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdResetErrors           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdPwm                   (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdSwitch                (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdRelay                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdMultiSwitch           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdMeshCommand           (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdEnableContPowerMeasure(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdAllowDimming          (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdLockSwitch            (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdSetup                 (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);
	ERR_CODE handleCmdEnableSwitchcraft     (buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel);

};

