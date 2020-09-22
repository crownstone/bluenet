/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <uart/cs_UartConnection.h>

#include <storage/cs_State.h>

UartConnection::UartConnection() {

}

void UartConnection::init() {
	TYPIFY(STATE_OPERATION_MODE) opMode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &opMode, sizeof(opMode));

	_status.flags.flags.hasBeenSetup = (static_cast<OperationMode>(opMode) == OperationMode::OPERATION_MODE_NORMAL);
	_status.flags.flags.hasError = false;

	// TODO: check if UART keys are set.
	_status.flags.flags.encryptionRequired = false;

	listen();
}

const uart_msg_status_reply_t& UartConnection::getSelfStatus() {
	return _status;
}

const uart_msg_status_user_t& UartConnection::getUserStatus() {
	return _userStatus;
}

void UartConnection::onUserStatus(const uart_msg_status_user_t& status) {
	_userStatus = status;
}

void UartConnection::onHeartBeat(uint16_t timeoutSeconds) {
	_isConnectionAlive = true;
	_connectionTimeoutCountdown = timeoutSeconds * (1000 / TICK_INTERVAL_MS);
}

void UartConnection::onTick() {
	if (_connectionTimeoutCountdown) {
		--_connectionTimeoutCountdown;
		if (_connectionTimeoutCountdown == 0) {
			// No heartbeat received within timeout: connection died.
			_isConnectionAlive = false;
		}
	}
}

void UartConnection::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_TICK:
			onTick();
			break;
		// TODO : event UART key set.
		default:
			break;
	}
}
