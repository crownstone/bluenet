/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 22, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"

/**
 * Item with state of external stone.
 *
 * timeoutCount         Count until this state is considered timed out.
 *                      Every item with timeoutCount of 0, is considered empty.
 * id                   Id of the stone.
 * state                The state of the stone.
 */
struct __attribute__((__packed__)) cs_external_state_item_t {
	uint16_t timeoutCount;
	stone_id_t id;
	state_external_stone_t state;
};

/**
 * Class that keeps up states of other stones.
 *
 * This includes:
 * - Storing the states of other stones.
 * - Keeping up if states are timed out.
 * - Choosing which state should be broadcasted next.
 */
class ExternalStates {
public:
	void init();

	/**
	 * To be called when the state of another stone has been received.
	 */
	void receivedState(state_external_stone_t* extState);

	/**
	 * Get a state to be broadcasted.
	 *
	 * Returns NULL if no state was found.
	 */
	service_data_encrypted_t* getNextState();

	/**
	 * To be called every EVT_TICK.
	 */
	void tick(TYPIFY(EVT_TICK) tickCount);
private:
	cs_external_state_item_t* _states;

	/** Index of states to be broadcasted next */
	int _broadcastIndex = 0;

	void removeFromList(stone_id_t id);

	void addToList(int index, stone_id_t id, state_external_stone_t* state);

	void fixState(state_external_stone_t* state);

	int8_t getRssi(stone_id_t id);
};
