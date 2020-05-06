/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/**
 * Sent between crownstones in a mesh with TTL 0 in order to find out
 * what the rssi between each pair of nodes is.
 *
 * For now, ping messages are echoed once when all information is gathered
 * so that all rssi-pairs can be recorded using a single crownstone for
 * debugging purpose.
 */
struct rssi_ping_message_t {
//	uint32_t sender_local_uptime;
//	uint32_t sender_local_systime;
	uint16_t sender_id;
	uint16_t recipient_id;
	uint8_t sample_id;
	uint8_t rssi;
};
