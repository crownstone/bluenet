/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 31, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <behaviour/cs_Behaviour.h>

#include <cstdint>

template <class T>
class CmdReplaceBehaviourData {
public:
	uint8_t _index;
	T _behaviour;

	CmdReplaceBehaviourData() = default;
};
