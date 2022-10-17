/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 17, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Describes a recent change event of the behaviour store.
 * See CS_TYPES::EVT_BEHAVIOURSTORE_MUTATION
 */
class BehaviourMutation {
public:
	enum Mutation { NONE, REMOVE, ADD, UPDATE };

	uint8_t _index;
	Mutation _mutation;

	BehaviourMutation() : _index(0xFF), _mutation(Mutation::NONE) {}
	BehaviourMutation(uint8_t index, Mutation mutation) : _index(index), _mutation(mutation) {}
};
