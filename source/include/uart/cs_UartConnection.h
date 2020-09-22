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

	void init();

	bool isEncryptionRequired();

	const uart_msg_status_reply_t& getSelfStatus();

	const uart_msg_status_user_t& getUserStatus();

	void onHeartBeat(uint16_t timeoutSeconds);

	void onUserStatus(const uart_msg_status_user_t& status);


private:
	//! Constructor
	UartConnection();

	//! This class is singleton, deny implementation
	UartConnection(UartConnection const&) = delete;

	//! This class is singleton, deny implementation
	void operator=(UartConnection const &) = delete;

	uart_msg_status_reply_t _status;

	bool _isConnectionAlive = false;

	uart_msg_status_user_t _userStatus;

	/**
	 * Timeout (in ticks) set by heartbeat.
	 * When this reaches 0, consider the connection te be dead.
	 */
	uint32_t _connectionTimeoutCountdown = 0;

	void onTick();

	void handleEvent(event_t & event);
};


