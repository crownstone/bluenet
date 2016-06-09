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

using namespace BLEpp;

class CommandHandler {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static CommandHandler& getInstance() {
		static CommandHandler instance;
		return instance;
	}

	void setStack(Nrf51822BluetoothStack* stack) {
		_stack = stack;
	}

	void handleCommand(CommandHandlerTypes type);

	void handleCommand(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size);

private:

	CommandHandler() : _stack(NULL) {}

	Nrf51822BluetoothStack* _stack;

};

