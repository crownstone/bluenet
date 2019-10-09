/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

/**
 * Struct to communicate state variables.
 *
 * type       The state type.
 * value      Pointer to the state value.
 * size       Size of the state value. When getting a state, this should be set to available size of value pointer.
 *            Afterwards, it will be set to the size of the state value.
 */
struct cs_state_data_t {
	CS_TYPE type;
	uint8_t *value;
	size16_t size;

	friend bool operator==(const cs_state_data_t data0, const cs_state_data_t & data1) {
		if (data0.type != data1.type || data0.size != data1.size) {
			return false;
		}
		for (int i = 0; i < data0.size; ++i) {
			if (data0.value[i] != data1.value[i]) {
				return false;
			}
		}
		return true;
	}
	friend bool operator!=(const cs_state_data_t data0, const cs_state_data_t & data1) {
		return !(data0 == data1);
	}
	cs_state_data_t():
		type(CS_TYPE::CONFIG_DO_NOT_USE),
		value(NULL),
		size(0)
	{}
	cs_state_data_t(CS_TYPE type, uint8_t *value, size16_t size):
		type(type),
		value(value),
		size(size)
	{}

};

/** Gets the default.
 *
 * Note that if data.value is not aligned at a word boundary, the result still isn't.
 *
 * There is no allocation done in this function. It is assumed that data.value points to an array or single
 * variable that needs to be written. The allocation of strings or arrays is limited by TypeSize which in that case
 * can be considered as MaxTypeSize.
 *
 * This function does not check if data size fits the default value.
 * TODO: check how to check this at compile time.
 */
cs_ret_code_t getDefault(cs_state_data_t & data, const boards_config_t& boardsConfig);