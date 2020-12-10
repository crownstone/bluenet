/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>


/**
 * Sent between crownstones in a mesh with TTL 0 in order to find out
 * what the rssi between each pair of nodes is.
 *
 * For now, ping messages are echoed once when all information is gathered
 * so that all rssi-pairs can be recorded using a single crownstone for
 * debugging purpose.
 */
struct __attribute__((__packed__)) rssi_ping_message_t {
	stone_id_t sender_id;
	int8_t rssi;
	uint8_t channel;
};
