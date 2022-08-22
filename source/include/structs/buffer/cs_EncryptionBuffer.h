/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <structs/buffer/cs_CharacteristicBuffer.h>

class EncryptionBuffer : public CharacteristicBuffer {
public:
	static EncryptionBuffer& getInstance() {
		static EncryptionBuffer instance;
		return instance;
	}
};
