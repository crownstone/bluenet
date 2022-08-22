/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "structs/buffer/cs_CharacteristicBuffer.h"

/**
 * Buffer used to hold plain text data that is read by the user.
 */
class CharacteristicReadBuffer : public CharacteristicBuffer {
public:
	static CharacteristicReadBuffer& getInstance() {
		static CharacteristicReadBuffer instance;
		return instance;
	};
};
