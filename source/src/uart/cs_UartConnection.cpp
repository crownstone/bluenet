/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <drivers/cs_RNG.h>
#include <storage/cs_State.h>
#include <uart/cs_UartConnection.h>
#include <uart/cs_UartHandler.h>

#define LOGUartconnectionDebug LOGnone

UartConnection::UartConnection() {}

void UartConnection::init() {
	// Init status flags.

	std::vector<cs_state_id_t>* ids = nullptr;
	State::getInstance().getIds(CS_TYPE::STATE_UART_KEY, ids);
	_status.flags.flags.encryptionRequired = (ids->empty() == false);

	TYPIFY(STATE_OPERATION_MODE) opMode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &opMode, sizeof(opMode));
	_status.flags.flags.hasBeenSetUp = (static_cast<OperationMode>(opMode) == OperationMode::OPERATION_MODE_NORMAL);

	TYPIFY(STATE_HUB_MODE) hubMode;
	State::getInstance().get(CS_TYPE::STATE_HUB_MODE, &hubMode, sizeof(hubMode));
	_status.flags.flags.hubMode  = (hubMode != 0);

	_status.flags.flags.hasError = false;

	listen();
}

bool UartConnection::isAlive() {
	return _isConnectionAlive;
}

bool UartConnection::isEncryptedAlive() {
	return _isConnectionAlive && _isConnectionEncrypted;
}

const uart_msg_status_reply_t& UartConnection::getSelfStatus() {
	return _status;
}

const uart_msg_status_user_t& UartConnection::getUserStatus() {
	return _userStatus;
}

void UartConnection::onHello(const uart_msg_status_user_flags_t& flags) {
	LOGUartconnectionDebug("Set user flags");
	_userStatus.flags = flags;

	TYPIFY(CONFIG_SPHERE_ID) sphereId;
	State::getInstance().get(CS_TYPE::CONFIG_SPHERE_ID, &sphereId, sizeof(sphereId));

	uart_msg_hello_t hello;
	hello.sphereId = sphereId;
	hello.status   = _status;
	LOGd("Reply hello with sphereId=%u, flags=%u", hello.sphereId, hello.status.flags.asInt);
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_HELLO, (uint8_t*)&hello, sizeof(hello));
}

void UartConnection::onUserStatus(const uart_msg_status_user_t& status) {
	LOGUartconnectionDebug("Set user status");
	_userStatus = status;

	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_STATUS, (uint8_t*)&_status, sizeof(_status));
}

void UartConnection::onHeartBeat(uint16_t timeoutSeconds, bool encrypted) {
	LOGUartconnectionDebug("Heartbeat timeout=%u", timeoutSeconds);
	_isConnectionAlive            = true;
	_isConnectionEncrypted        = encrypted;
	_connectionTimeoutCountdown   = timeoutSeconds * (1000 / TICK_INTERVAL_MS);

	// Reply with a heartbeat, which is only encrypted if the received heartbeat is encrypted.
	UartProtocol::Encrypt encrypt = encrypted ? UartProtocol::ENCRYPT_OR_FAIL : UartProtocol::Encrypt::ENCRYPT_NEVER;
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_HEARTBEAT, nullptr, 0, encrypt);
}

void UartConnection::onSessionNonce(const uart_msg_session_nonce_t& sessionNonce) {
	LOGUartconnectionDebug(
			"SessionNonce timeout=%u nonce=[%u %u .. %u]",
			sessionNonce.timeoutMinutes,
			sessionNonce.sessionNonce[0],
			sessionNonce.sessionNonce[1],
			sessionNonce.sessionNonce[SESSION_NONCE_LENGTH - 1]);
	memcpy(_sessionNonceRx, sessionNonce.sessionNonce, SESSION_NONCE_LENGTH);
	_sessionNonceValid            = true;
	_sessionNonceTimeoutCountdown = sessionNonce.timeoutMinutes * 60 * (1000 / TICK_INTERVAL_MS);

	// Refresh our own session nonce.
	RNG::fillBuffer(_sessionNonceTx, sizeof(_sessionNonceTx));

	// Reply with our new session nonce.
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_SESSION_NONCE, _sessionNonceTx, sizeof(_sessionNonceTx));
}

cs_ret_code_t UartConnection::getSessionNonceTx(cs_data_t data) {
	if (data.data == nullptr || data.len < SESSION_NONCE_LENGTH) {
		return ERR_BUFFER_TOO_SMALL;
	}
	if (!_sessionNonceValid) {
		return ERR_NOT_FOUND;
	}
	memcpy(data.data, _sessionNonceTx, SESSION_NONCE_LENGTH);
	return ERR_SUCCESS;
}

cs_ret_code_t UartConnection::getSessionNonceRx(cs_data_t data) {
	if (data.data == nullptr || data.len < SESSION_NONCE_LENGTH) {
		return ERR_BUFFER_TOO_SMALL;
	}
	if (!_sessionNonceValid) {
		return ERR_NOT_FOUND;
	}
	memcpy(data.data, _sessionNonceRx, SESSION_NONCE_LENGTH);
	return ERR_SUCCESS;
}

void UartConnection::onTick() {
	if (_connectionTimeoutCountdown) {
		--_connectionTimeoutCountdown;
		if (_connectionTimeoutCountdown == 0) {
			LOGi("Connection timed out");
			// No heartbeat received within timeout: connection died.
			_isConnectionAlive     = false;
			_isConnectionEncrypted = false;
		}
	}

	if (_sessionNonceTimeoutCountdown) {
		--_sessionNonceTimeoutCountdown;
		if (_sessionNonceTimeoutCountdown == 0) {
			LOGi("Session nonce timed out");
			_sessionNonceValid = false;
		}
	}
}

void UartConnection::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_TICK: onTick(); break;
		case CS_TYPE::STATE_HUB_MODE: {
			TYPIFY(STATE_HUB_MODE)* hubMode = reinterpret_cast<TYPIFY(STATE_HUB_MODE)*>(event.data);
			_status.flags.flags.hubMode     = (*hubMode != 0);
			break;
		}
		case CS_TYPE::STATE_UART_KEY: {
			// TODO : actually check if UART key are set?
			_status.flags.flags.encryptionRequired = true;
			break;
		}
		default: break;
	}
}
