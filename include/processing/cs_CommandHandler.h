/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 18, 2016
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <common/cs_Types.h>
#include <protocol/cs_CommandTypes.h>

#include <ble/cs_Stack.h>

extern "C" {
	#include <third/protocol/rbc_mesh.h>
}

struct delayed_command_t {
	CommandHandlerTypes type;
	uint16_t size;
	buffer_ptr_t buffer;
};

class CommandHandler {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static CommandHandler& getInstance() {
		static CommandHandler instance;
		return instance;
	}

	void init();

	ERR_CODE handleCommand(CommandHandlerTypes type);

	ERR_CODE handleCommand(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel accessLevel = ADMIN);

	ERR_CODE handleCommandDelayed(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size, uint32_t delay);

	void resetDelayed(uint8_t opCode);

private:

	CommandHandler();

	app_timer_id_t _delayTimer;

};

