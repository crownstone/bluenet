/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <behaviour/cs_ExtendedSwitchBehaviour.h>
#include <behaviour/cs_SwitchBehaviour.h>
#include <behaviour/cs_TwilightBehaviour.h>

#include <events/cs_EventListener.h>
#include <protocol/cs_ErrorCodes.h>
#include <common/cs_Component.h>

#include <array>
#include <optional>
#include <vector>

/**
 * Keeps track of the behaviours that are active on this crownstone.
 */
class BehaviourStore : public EventListener, public Component {
public:
	static constexpr size_t MaxBehaviours = 50;
private:
	std::array<Behaviour*, MaxBehaviours> activeBehaviours = {};

public:
	/**
	 * handles events concerning updates of the active behaviours on this crownstone.
	 */
	virtual void handleEvent(event_t& evt);

	/*****************************
	 * NOTE: to loop over a specific type of behaviours simply do:
	 *
	 * for(auto b : getActiveBehaviours()){
	 *   if(auto switchbehave = dynamic_cast<SwitchBehaviour*>(b)){
	 *     // work with switchbehave
	 *   }
	 * }
	 */
	inline std::array<Behaviour*, MaxBehaviours>& getActiveBehaviours() {
		return activeBehaviours;
	}

	/**
	 * Initialize store from flash.
	 */
	cs_ret_code_t init() override;
	
	virtual ~BehaviourStore();

private:
	/**
	 * Add behaviour to a new index.
	 *
	 * Does not check if similar behaviour already exists.
	 *
	 * @param[in] buf        Buffer with the behaviour.
	 * @param[in] bufSize    Size of the buffer.
	 * @param[out] index     Index at which the behaviour was added, only set on success.
	 * @return error code.
	 */
	ErrorCodesGeneral addBehaviour(uint8_t* buf, cs_buffer_size_t bufSize, uint8_t & index);

	/**
	 * Remove the behaviour at [index]. If [index] is out of bounds,
	 * or no behaviour exists at [index], false is returned. Else, true.
	 */
	ErrorCodesGeneral removeBehaviour(uint8_t index);

	/**
	 * Clear all behaviours - usefull for a clean start of the test suite.
	 * Does not edit the persisted values, so expect discrepancies with
	 * flash until all behaviours in flash are overwritten.
	 */
	void clearActiveBehavioursArray();

	/**
	 * Calculate the hash over all behaviours.
	 */
	uint32_t calculateMasterHash();

	/**
	 * Calculate master hash and store it in State.
	 */
	void storeMasterHash();

	void handleSaveBehaviour(event_t& evt);
	void handleReplaceBehaviour(event_t& evt);
	void handleRemoveBehaviour(event_t& evt);
	void handleGetBehaviour(event_t& evt);
	void handleGetBehaviourIndices(event_t& evt);

	// utilities
	void dispatchBehaviourMutationEvent();

	// checks intermediate state of handleReplaceBehaviour for consistency.
	// returns true if ok, false if nok.
	bool ReplaceParameterValidation(event_t& evt, uint8_t index, const size_t& behaviourSize);

	// loads the behaviours from state into the 'activeBehaviours' array.
	// BehaviourType must match BehaviourCsType.
	template<class BehaviourType>
	void LoadBehavioursFromMemory(CS_TYPE BehaviourCsType);
};
