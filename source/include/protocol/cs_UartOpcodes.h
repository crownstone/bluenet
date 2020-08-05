/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 18, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once


enum UartOpcodeRx {
	UART_OPCODE_RX_CONTROL =                          1,
	UART_OPCODE_RX_ENABLE_ADVERTISEMENT =             10000, // Enable advertising (payload: bool enable)
	UART_OPCODE_RX_ENABLE_MESH =                      10001, // Enable mesh (payload: bool enable)
	UART_OPCODE_RX_GET_ID =                           10002, // Get ID of this Crownstone
	UART_OPCODE_RX_GET_MAC =                          10003, // Get MAC address of this Crownstone

//	UART_OPCODE_RX_ADC_CONFIG_GET =                   10100, // Get the adc config
//	UART_OPCODE_RX_ADC_CONFIG_SET =                   10101, // Set an adc channel config (payload: uart_msg_adc_channel_config_t)
//	UART_OPCODE_RX_ADC_CONFIG_SET_RANGE =             10102, // Set range of given channel (payload: channel, range)
	UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_CURRENT =     10103, // Increase the range on the current channel
	UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_CURRENT =     10104, // Decrease the range on the current channel
	UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_VOLTAGE =     10105, // Increase the range on the voltage channel
	UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_VOLTAGE =     10106, // Decrease the range on the voltage channel
	UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_CURRENT =  10108, // Enable differential mode on current channel (payload: bool enable)
	UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_VOLTAGE =  10109, // Enable differential mode on voltage channel (payload: bool enable)
	UART_OPCODE_RX_ADC_CONFIG_VOLTAGE_PIN =           10110, // Change the pin used on voltage channel (payload: enum of pins)

	UART_OPCODE_RX_POWER_LOG_CURRENT =                10200, // Enable writing current samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_VOLTAGE =                10201, // Enable writing voltage samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_FILTERED_CURRENT =       10202, // Enable writing filtered current samples (payload: bool enable)
//	UART_OPCODE_RX_POWER_LOG_FILTERED_VOLTAGE =       10203, // Enable writing filtered voltage samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_POWER =                  10204, // Enable writing calculated power (payload: bool enable)
};

enum UartOpcodeTx {
	UART_OPCODE_TX_ACK =                              0,
	UART_OPCODE_TX_CONTROL_RESULT =                   1, // The result of the control command, payload: result_packet_header_t + data.
	UART_OPCODE_TX_SERVICE_DATA =                     2, // Sent when the service data is updated (payload: service_data_t)
	UART_OPCODE_TX_BLE_MSG =                          3, // Sent by command (CMD_UART_MSG), payload: buffer.
	UART_OPCODE_TX_MESH_STATE =                       102, // Received state of external stone, payload: service_data_encrypted_t
	UART_OPCODE_TX_MESH_STATE_PART_0 =                103, // Received part of state of external stone, payload: cs_mesh_model_msg_state_0_t
	UART_OPCODE_TX_MESH_STATE_PART_1 =                104, // Received part of state of external stone, payload: cs_mesh_model_msg_state_1_t
	UART_OPCODE_TX_MESH_RESULT =                      105, // Received the result of a mesh command, payload: uart_msg_mesh_result_packet_header_t + data.
	UART_OPCODE_TX_MESH_ACK_ALL_RESULT =              106, // Whether all stone IDs were acked, payload: result_packet_header_t.

	UART_OPCODE_TX_ADVERTISEMENT_ENABLED =            10000, // Whether advertising is enabled (payload: bool)
	UART_OPCODE_TX_MESH_ENABLED =                     10001, // Whether mesh is enabled (payload: bool)
	UART_OPCODE_TX_OWN_ID =                           10002, // Own id (payload: crownstone_id_t)
	UART_OPCODE_TX_OWN_MAC =                          10003, // Own MAC address (payload: mac address (6B))

	UART_OPCODE_TX_ADC_CONFIG =                       10100, // Current adc config (payload: adc_config_t)
	UART_OPCODE_TX_ADC_RESTART =                      10101,

	UART_OPCODE_TX_POWER_LOG_CURRENT =                10200,
	UART_OPCODE_TX_POWER_LOG_VOLTAGE =                10201,
	UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT =       10202,
	UART_OPCODE_TX_POWER_LOG_FILTERED_VOLTAGE =       10203,
	UART_OPCODE_TX_POWER_LOG_POWER =                  10204,


	UART_OPCODE_TX_EVT =                              10300, // Send internal events, this protocol may change

	UART_OPCODE_TX_TEXT =                             20000, // Payload is ascii text.
	UART_OPCODE_TX_FIRMWARESTATE =                    20001,
};
