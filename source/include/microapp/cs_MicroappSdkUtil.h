/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 29, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <protocol/cs_Typedefs.h>
#include <cs_MicroappStructs.h>
#include <ble/cs_UUID.h>

class MicroappSdkUtil {
public:
	/**
	 * Maps digital pins to interrupts.
	 */
	static uint8_t digitalPinToInterrupt(uint8_t pinIndex);

	/**
	 * Maps interrupts to digital pins.
	 */
	static uint8_t interruptToDigitalPin(uint8_t interrupt);

	/**
	 * Maps a bluenet return code to a microapp ack code.
	 */
	static MicroappSdkAck bluenetResultToMicroapp(cs_ret_code_t retCode);


	static microapp_sdk_ble_uuid_t convertUuid(const UUID& uuid);
};

