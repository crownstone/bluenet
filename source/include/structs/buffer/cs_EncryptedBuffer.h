/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <structs/buffer/cs_CharacteristicBuffer.h>

/**
 * Buffer used to hold encrypted characteristic data.
 */
class EncryptedBuffer : public CharacteristicBuffer {
public:
	static EncryptedBuffer& getInstance() {
		static EncryptedBuffer instance;
		return instance;
	}
};
