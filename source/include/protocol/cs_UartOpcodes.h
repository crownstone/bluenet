/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 18, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Messages received over UART. Note that the documentation on github is from the perspective of the user.
 *   https://github.com/crownstone/bluenet/blob/master/docs/UART_PROTOCOL.md
 * Hence, what is called RX here is called TX there.
 */
enum UartOpcodeRx {
	UART_OPCODE_RX_HELLO =                            0,
	UART_OPCODE_RX_SESSION_NONCE =                    1,
	UART_OPCODE_RX_HEARTBEAT =                        2,
	UART_OPCODE_RX_STATUS =                           3,
	UART_OPCODE_RX_GET_MAC =                          4, // Get MAC address of this Crownstone
	UART_OPCODE_RX_CONTROL =                          10,
	UART_OPCODE_RX_HUB_DATA_REPLY =                   11, // Payload starts with uart_msg_hub_data_reply_header_t.

	////////// Developer messages in debug build. //////////
	UART_OPCODE_RX_ENABLE_ADVERTISEMENT =             50000, // Enable advertising (payload: bool enable)
	UART_OPCODE_RX_ENABLE_MESH =                      50001, // Enable mesh (payload: bool enable)
	UART_OPCODE_RX_GET_ID =                           50002, // Get ID of this Crownstone

//	UART_OPCODE_RX_ADC_CONFIG_GET =                   50100, // Get the adc config
//	UART_OPCODE_RX_ADC_CONFIG_SET =                   50101, // Set an adc channel config (payload: uart_msg_adc_channel_config_t)
//	UART_OPCODE_RX_ADC_CONFIG_SET_RANGE =             50102, // Set range of given channel (payload: channel, range)
	UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_CURRENT =     50103, // Increase the range on the current channel
	UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_CURRENT =     50104, // Decrease the range on the current channel
	UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_VOLTAGE =     50105, // Increase the range on the voltage channel
	UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_VOLTAGE =     50106, // Decrease the range on the voltage channel
	UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_CURRENT =  50108, // Enable differential mode on current channel (payload: bool enable)
	UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_VOLTAGE =  50109, // Enable differential mode on voltage channel (payload: bool enable)
	UART_OPCODE_RX_ADC_CONFIG_VOLTAGE_PIN =           50110, // Change the pin used on voltage channel (payload: enum of pins)

	UART_OPCODE_RX_POWER_LOG_CURRENT =                50200, // Enable writing current samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_VOLTAGE =                50201, // Enable writing voltage samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_FILTERED_CURRENT =       50202, // Enable writing filtered current samples (payload: bool enable)
//	UART_OPCODE_RX_POWER_LOG_FILTERED_VOLTAGE =       50203, // Enable writing filtered voltage samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_POWER =                  50204, // Enable writing calculated power (payload: bool enable)

	UART_OPCODE_RX_INJECT_EVENT =                     60000, // Dispatch any event. Payload: CS_TYPE + event data structure.
};

/**
 * Send messages over the UART to someone listening.
 *
 * RX on https://github.com/crownstone/bluenet/blob/master/docs/UART_PROTOCOL.md.
 */
enum UartOpcodeTx {
	UART_OPCODE_TX_HELLO =                            0,
	UART_OPCODE_TX_SESSION_NONCE =                    1,
	UART_OPCODE_TX_HEARTBEAT =                        2,
	UART_OPCODE_TX_STATUS =                           3,
	UART_OPCODE_TX_MAC =                              4,  // MAC address (payload: mac address (6B))
	UART_OPCODE_TX_CONTROL_RESULT =                   10, // The result of the control command, payload: result_packet_header_t + data.
	UART_OPCODE_TX_HUB_DATA_REPLY_ACK =               11,


	////////// Error replies. //////////
	UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED =         9900, // Replied on error, the command probably has a wrong format, or is too large.
	UART_OPCODE_TX_ERR_REPLY_STATUS =                 9901, // Replied on error, the command probably should have been encrypted.
	UART_OPCODE_TX_ERR_REPLY_SESSION_NONCE_MISSING =  9902, // Replied when RX session nonce is missing.
	UART_OPCODE_TX_ERR_REPLY_DECRYPTION_FAILED =      9903, // Replied when decryption failed due to missing or wrong key.

	////////// Event messages. //////////
	UART_OPCODE_TX_BLE_MSG =                          10000, // Sent by command (CMD_UART_MSG), payload: buffer.
	UART_OPCODE_TX_SESSION_NONCE_MISSING =            10001, // Sent when TX session nonce is missing.
	UART_OPCODE_TX_SERVICE_DATA =                     10002, // Sent when the service data is updated (payload: service_data_t)
//	UART_OPCODE_TX_DECRYPTION_FAILED =                10003, // Sent when decryption failed due to missing or wrong key.
	UART_OPCODE_TX_PRESENCE_CHANGE =                  10004, // Sent when the presence has changed, payload: presence_change_t. When the first user enters, multiple msgs will be sent.
	UART_OPCODE_TX_FACTORY_RESET =                    10005, // Sent when a factory reset is going to be performed.
	UART_OPCODE_TX_BOOTED =                           10006, // Sent when this crownstone just booted.
	UART_OPCODE_TX_HUB_DATA =                         10007, // Sent by command (CTRL_CMD_HUB_DATA), payload: buffer.

	UART_OPCODE_TX_MESH_STATE =                       10102, // Received state of external stone, payload: service_data_encrypted_t
	UART_OPCODE_TX_MESH_STATE_PART_0 =                10103, // Received part of state of external stone, payload: cs_mesh_model_msg_state_0_t
	UART_OPCODE_TX_MESH_STATE_PART_1 =                10104, // Received part of state of external stone, payload: cs_mesh_model_msg_state_1_t
	UART_OPCODE_TX_MESH_RESULT =                      10105, // Received the result of a mesh command, payload: uart_msg_mesh_result_packet_header_t + data.
	UART_OPCODE_TX_MESH_ACK_ALL_RESULT =              10106, // Whether all stone IDs were acked, payload: result_packet_header_t.
	UART_OPCODE_TX_RSSI_DATA_MESSAGE =                10107, // When MeshTopologyResearch receives an rssi_data_message, it is sent to host. Payload: RssiDataMessage.
	UART_OPCODE_TX_ASSET_RSSI_MAC_DATA =              10108, // Payload: cs_asset_rssi_mac_data_t. Info about an asset a Crownstone on the mesh has forwarded.
	UART_OPCODE_TX_NEAREST_CROWNSTONE_UPDATE =        10109, // Payload: cs_nearest_stone_update_t. The rssi between an asset and its nearest Crownstone changed.
	UART_OPCODE_TX_NEIGHBOUR_RSSI =                   10111, // Payload: mesh_topology_neighbour_rssi_t
	UART_OPCODE_TX_ASSET_RSSI_SID_DATA =              10112, // Payload: cs_asset_rssi_sid_data_t. Info about an asset a Crownstone on the mesh has forwarded.


	UART_OPCODE_TX_LOG =                              10200, // Debug logs, payload is in the form: [uart_msg_log_header_t, [uart_msg_log_arg_header_t, data], [uart_msg_log_arg_header_t, data], ...]
	UART_OPCODE_TX_LOG_ARRAY =                        10201, // Debug logs, payload is in the form: [uart_msg_log_header_t, [uart_msg_log_arg_header_t, data], [uart_msg_log_arg_header_t, data], ...]


	////////// Developer messages in release builds. //////////
	UART_OPCODE_TX_EVT =                              40000, // Send internal events, this protocol may change

//	UART_OPCODE_TX_MESH_STATE_TIME =                  40102, // Received time of other crownstone, payload: cs_mesh_model_msg_time_t
	UART_OPCODE_TX_MESH_CMD_TIME =                    40103, // Received cmd to set time, payload: cs_mesh_model_msg_time_t
	UART_OPCODE_TX_MESH_PROFILE_LOCATION =            40110, // Received the location of a profile, payload: cs_mesh_model_msg_profile_location_t.
	UART_OPCODE_TX_MESH_SET_BEHAVIOUR_SETTINGS =      40111, // Received cmd to set behaviour settings, payload: behaviour_settings_t
	UART_OPCODE_TX_MESH_TRACKED_DEVICE_REGISTER =     40112, // Received cmd to register a tracked device, payload: cs_mesh_model_msg_device_register_t
	UART_OPCODE_TX_MESH_TRACKED_DEVICE_TOKEN =        40113, // Received cmd to set the token of a tracked device, payload: cs_mesh_model_msg_device_token_t
	UART_OPCODE_TX_MESH_SYNC_REQUEST =                40114, // Received a sync request, payload: cs_mesh_model_msg_sync_request_t
	UART_OPCODE_TX_MESH_TRACKED_DEVICE_HEARTBEAT =    40120, // Received heartbeat cmd of a tracked device, payload: cs_mesh_model_msg_device_heartbeat_t


	////////// Developer messages in debug builds. //////////
	UART_OPCODE_TX_ADVERTISEMENT_ENABLED =            50000, // Whether advertising is enabled (payload: bool)
	UART_OPCODE_TX_MESH_ENABLED =                     50001, // Whether mesh is enabled (payload: bool)
	UART_OPCODE_TX_OWN_ID =                           50002, // Own id (payload: crownstone_id_t)

	UART_OPCODE_TX_ADC_CONFIG =                       50100, // Current adc config (payload: adc_config_t)
	UART_OPCODE_TX_ADC_RESTART =                      50101,

	UART_OPCODE_TX_POWER_LOG_CURRENT =                50200,
	UART_OPCODE_TX_POWER_LOG_VOLTAGE =                50201,
	UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT =       50202,
	UART_OPCODE_TX_POWER_LOG_FILTERED_VOLTAGE =       50203,
	UART_OPCODE_TX_POWER_LOG_POWER =                  50204,

	UART_OPCODE_TX_TEXT =                             60000, // Payload is ascii text.
	UART_OPCODE_TX_FIRMWARESTATE =                    60001,
};

namespace UartProtocol {

/**
 * Option whether a UART message should be encrypted.
 */
enum Encrypt {
	ENCRYPT_NEVER = 0,                 // Never encrypt the message.
	ENCRYPT_IF_ENABLED = 1,            // Encrypt the message when encryption is enabled
	ENCRYPT_OR_FAIL = 2,               // Encrypt the message or return an error
	ENCRYPT_ACCORDING_TO_TYPE = 3,     // Encrypt depending on data type (UartOpcodeTx).
};

/**
 * Whether a received UART message must be encrypted when "encryption required" is true (when a UART key is set).
 *
 * Optionals should return false.
 */
constexpr bool mustBeEncryptedRx(UartOpcodeRx opCode) {
	switch (opCode) {
		case UartOpcodeRx::UART_OPCODE_RX_HELLO:
		case UartOpcodeRx::UART_OPCODE_RX_SESSION_NONCE:
		case UartOpcodeRx::UART_OPCODE_RX_HEARTBEAT: // optional
		case UartOpcodeRx::UART_OPCODE_RX_STATUS: // optional
		case UartOpcodeRx::UART_OPCODE_RX_GET_MAC:
		case UartOpcodeRx::UART_OPCODE_RX_HUB_DATA_REPLY: // optional
			return false;
		default:
			if (opCode >= 50000) {
				return false;
			}
			return true;
	}
}

/**
 * Whether a written UART message must be encrypted when "encryption required" is true (when a UART key is set).
 *
 * Optionals should return true.
 */
constexpr bool mustBeEncryptedTx(UartOpcodeTx opCode) {
	switch (opCode) {
		case UartOpcodeTx::UART_OPCODE_TX_HELLO:
		case UartOpcodeTx::UART_OPCODE_TX_SESSION_NONCE:
		case UartOpcodeTx::UART_OPCODE_TX_STATUS:
		case UartOpcodeTx::UART_OPCODE_TX_MAC:
		case UartOpcodeTx::UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED:
		case UartOpcodeTx::UART_OPCODE_TX_ERR_REPLY_STATUS:
		case UartOpcodeTx::UART_OPCODE_TX_ERR_REPLY_SESSION_NONCE_MISSING:
		case UartOpcodeTx::UART_OPCODE_TX_ERR_REPLY_DECRYPTION_FAILED:
		case UartOpcodeTx::UART_OPCODE_TX_SESSION_NONCE_MISSING:
		case UartOpcodeTx::UART_OPCODE_TX_BOOTED:
			return false;
		default:
			if (opCode >= 50000) {
				return false;
			}
			return true;
	}
}

}
