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
	enum Mutation { NONE, REMOVE, ADD, UPDATE, CLEAR_ALL };

	uint8_t _index;
	Mutation _mutation;

	BehaviourMutation(uint8_t index, Mutation mutation) : _index(index), _mutation(mutation) {}
	BehaviourMutation(Mutation mutation) : BehaviourMutation(0xFF, mutation) {}
	BehaviourMutation() : BehaviourMutation(0xFF, Mutation::NONE) {}
};
