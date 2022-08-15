/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_UartProtocol.h>

/**
 * Class that:
 * - Keeps up the UART status.
 * - Checks heartbeat to see if connection is alive.
 * - Keeps up the session nonce (also checks if it's timed out).
 */
class UartConnection : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation) of this class.
	static UartConnection& getInstance() {
		static UartConnection instance;
		return instance;
	}

	/**
	 * Initialize the class.
	 *
	 * - Reads settings from State.
	 * - Starts listening for events.
	 */
	void init();

	/**
	 * Returns whether the connection is considered to be alive (receiving heartbeats).
	 */
	bool isAlive();

	/**
	 * Returns whether the connection is considered to be alive and encrypted (receiving encrypted heartbeats).
	 */
	bool isEncryptedAlive();

	/**
	 * Returns the UART status reply.
	 */
	const uart_msg_status_reply_t& getSelfStatus();

	/**
	 * Returns the status of the UART user.
	 */
	const uart_msg_status_user_t& getUserStatus();

	/**
	 * To be called on UART hello command.
	 *
	 * Sends reply.
	 */
	void onHello(const uart_msg_status_user_flags_t& flags);

	/**
	 * To be called on UART heartbeat command.
	 *
	 * Sends reply.
	 */
	void onHeartBeat(uint16_t timeoutSeconds, bool encrypted);

	/**
	 * To be called on UART status command.
	 *
	 * Sends reply.
	 */
	void onUserStatus(const uart_msg_status_user_t& status);

	/**
	 * To be called on UART session nonce command.
	 *
	 * Sends reply.
	 */
	void onSessionNonce(const uart_msg_session_nonce_t& sessionNonce);

	/**
	 * Get the session nonce for TX.
	 *
	 * @param[out] data      Buffer to copy the session nonce to.
	 *
	 * @return               Result code.
	 */
	cs_ret_code_t getSessionNonceTx(cs_data_t data);

	/**
	 * Get the session nonce for RX.
	 *
	 * @param[out] data      Buffer to copy the session nonce to.
	 *
	 * @return               Result code.
	 */
	cs_ret_code_t getSessionNonceRx(cs_data_t data);

private:
	//! Constructor
	UartConnection();

	//! This class is singleton, deny implementation
	UartConnection(UartConnection const&) = delete;

	//! This class is singleton, deny implementation
	void operator=(UartConnection const&) = delete;

	//! Keep up the UART status reply.
	uart_msg_status_reply_t _status;

	//! Keep up the UART user status.
	uart_msg_status_user_t _userStatus;

	//! Keep up whether the connection is considered to be alive.
	bool _isConnectionAlive              = false;

	//! Whether the connection heartbeats are encrypted.
	bool _isConnectionEncrypted          = false;

	/**
	 * Timeout (in tick events) set by heartbeat.
	 * When this reaches 0, consider the connection te be dead.
	 */
	uint32_t _connectionTimeoutCountdown = 0;

	/**
	 * Session nonce used to decrypt incoming uart msgs.
	 */
	uint8_t _sessionNonceRx[SESSION_NONCE_LENGTH];

	/**
	 * Session nonce used to encrypt outgoing uart msgs.
	 */
	uint8_t _sessionNonceTx[SESSION_NONCE_LENGTH];

	/**
	 * Whether the RX and TX session nonce are valid.
	 */
	bool _sessionNonceValid                = false;

	/**
	 * Timeout (in tick events) set by the received session nonce.
	 * When this reaches 0, consider the RX and TX session nonce to be invalid.
	 */
	uint32_t _sessionNonceTimeoutCountdown = 0;

	void onTick();

	void handleEvent(event_t& event);
};
