/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>

/**
 * Struct to communicate state variables.
 *
 * type       The state type.
 * id         Multiple value per state type can exist, they all have a unique id. Default is 0.
 * value      Pointer to the state value.
 * size       Size of the state value. When getting a state, this should be set to available size of value pointer.
 *            Afterwards, it will be set to the size of the state value.
 */
struct __attribute__((packed)) cs_state_data_t {
	CS_TYPE type     = CS_TYPE::CONFIG_DO_NOT_USE;
	cs_state_id_t id = 0;
	uint8_t* value   = NULL;
	size16_t size    = 0;

	cs_state_data_t() {}
	cs_state_data_t(CS_TYPE type, uint8_t* value, size16_t size) : type(type), id(0), value(value), size(size) {}
	cs_state_data_t(CS_TYPE type, cs_state_id_t id, uint8_t* value, size16_t size)
			: type(type), id(id), value(value), size(size) {}
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
cs_ret_code_t getDefault(cs_state_data_t& data, const boards_config_t& boardsConfig);

/**
 * Store values in FLASH or RAM. Load values from FIRMWARE_DEFAULT, FLASH or RAM.
 *
 * 1. Values that are written fairly often and are not important over reboots should be stored in and read from RAM.
 * For example, we can measure continuously the temperature of the chip. We can also all the time read this value
 * from one of the BLE services. There is no reason to do a roundtrip to FLASH.
 *
 * 2. Values like CONFIG_BOOT_DELAY should be known over reboots of the device. Moreover, they should also persist
 * over firmware updates. These values are stored in FLASH. If the values are actually not changed by the user (or via
 * the smartphone app), they should NOT be stored to FLASH. They can then immediately be read from the
 * FIRMWARE_DEFAULT. They will be stored only if they are explicitly overwritten. If these values are stored in FLASH
 * they always take precedence over FIRMWARE_DEFAULT values.
 *
 * 3. Most values can be written to and read from FLASH using RAM as a cache. We can also use FIRMWARE_DEFAULT as
 * a fallback when the FLASH value is not present. Moreover, we can have a list that specifies if a value should be
 * in RAM or in FLASH by default. This complete persistence strategy is called STRATEGY1.
 *
 * NOTE. Suppose we have a new firmware available and we definitely want to use a new FIRMWARE_DEFAULT value. For
 * example, we use more peripherals and need to have a CONFIG_BOOT_DELAY that is higher or else it will be in an
 * infinite reboot loop. Before we upload the new firmware to the Crownstone, we need to explicitly clear the value.
 * Only after we have deleted the FLASH record we can upload the new firmware. Then the new FIRMWARE_DEFAULT is used
 * automatically.
 */
enum class PersistenceMode : uint8_t { FLASH, RAM, FIRMWARE_DEFAULT, STRATEGY1, NEITHER_RAM_NOR_FLASH };

PersistenceModeGet toPersistenceModeGet(uint8_t mode);
PersistenceModeSet toPersistenceModeSet(uint8_t mode);

PersistenceMode DefaultLocation(CS_TYPE const& type);
