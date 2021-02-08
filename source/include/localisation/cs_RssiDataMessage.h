/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 13, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

/**
 * Message format to be sent over uart.
 * This is the inflated counterpart of rssi_data_message_t.
 *
 * (Necessary since we have to fold in our own id anyway.)
 */
class RssiDataMessage {
	static constexpr uint8_t CHANNEL_COUNT = 3;
	public:
	stone_id_t receiver_id;
	stone_id_t sender_id;
	uint8_t count[CHANNEL_COUNT];
	uint8_t rssi[CHANNEL_COUNT];
	uint8_t standard_deviation[CHANNEL_COUNT];
};
