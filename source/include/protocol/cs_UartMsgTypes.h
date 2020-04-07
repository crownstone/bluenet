/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 17, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/**********************************************************************************************************************
 *
 * The Crownstone is a high-voltage (domestic) switch. It can be used for:
 *   - indoor localization
 *   - building automation
 *
 * It is one of the first, or the first(?), open-source Internet-of-Things devices entering the market.
 *
 * Read more on: https://crownstone.rocks
 *
 * Almost all configuration options should be set in CMakeBuild.config.
 *
 *********************************************************************************************************************/

#pragma once

#include "cfg/cs_Config.h"
#include "drivers/cs_ADC.h"
#include <cstdint>

struct __attribute__((__packed__)) uart_msg_mesh_result_packet_header_t {
	stone_id_t stoneId;
	result_packet_header_t resultHeader;
};

struct __attribute__((__packed__)) uart_msg_power_t {
	uint32_t timestamp;
	int32_t  currentRmsMA;
	int32_t  currentRmsMedianMA;
	int32_t  filteredCurrentRmsMA;
	int32_t  filteredCurrentRmsMedianMA;
	int32_t  avgZeroVoltage;
	int32_t  avgZeroCurrent;
	int32_t  powerMilliWattApparent;
	int32_t  powerMilliWattReal;
	int32_t  avgPowerMilliWattReal;
};

struct __attribute__((__packed__)) uart_msg_current_t {
	uint32_t timestamp;
	int16_t  samples[CS_ADC_BUF_SIZE/2];
};

struct __attribute__((__packed__)) uart_msg_voltage_t {
	uint32_t timestamp;
	int16_t  samples[CS_ADC_BUF_SIZE/2];
};

struct __attribute__((__packed__)) uart_msg_adc_channel_config_t {
	cs_adc_channel_id_t channel;
	adc_channel_config_t config;
};
