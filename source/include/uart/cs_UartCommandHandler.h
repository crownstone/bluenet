/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once
#include <protocol/cs_UartOpcodes.h>
#include <structs/cs_PacketsInternal.h>

class UartCommandHandler {
public:
	/**
	 * Handle UART command.
	 *
	 * - Checks access level.
	 * - Checks size of command data.
	 *
	 * @param[in] opCode          Command type.
	 * @param[in] commandData     Command payload data.
	 * @param[in] accessLevel     Access level.
	 * @param[in] wasEncrypted    Whether the incoming message was encrypted.
	 * @param[in] resultBuffer
	 */
	void handleCommand(
			UartOpcodeRx opCode,
			cs_data_t commandData,
			EncryptionAccessLevel accessLevel,
			bool wasEncrypted,
			cs_data_t resultBuffer);

private:
	EncryptionAccessLevel getRequiredAccessLevel(const UartOpcodeRx opCode);

	/**
	 * Dispatch event.
	 *
	 * @param[in] type            Event type.
	 * @param[in] commandData     Command data, which will be used as event data.
	 */
	void dispatchEventForCommand(CS_TYPE type, cs_data_t commandData);

	void handleCommandHello(cs_data_t commandData);
	void handleCommandSessionNonce(cs_data_t commandData);
	void handleCommandHeartBeat(cs_data_t commandData, bool wasEncrypted);
	void handleCommandStatus(cs_data_t commandData);
	void handleCommandControl(
			cs_data_t commandData,
			const cmd_source_with_counter_t source,
			const EncryptionAccessLevel accessLevel,
			cs_data_t resultBuffer);
	void handleCommandHubDataReply(
			cs_data_t commandData,
			const cmd_source_with_counter_t source,
			const EncryptionAccessLevel accessLevel,
			cs_data_t resultBuffer);
	void handleCommandEnableAdvertising(cs_data_t commandData);
	void handleCommandEnableMesh(cs_data_t commandData);
	void handleCommandGetId(cs_data_t commandData);
	void handleCommandGetMacAddress(cs_data_t commandData);
	void handleCommandInjectEvent(cs_data_t commandData);
};
