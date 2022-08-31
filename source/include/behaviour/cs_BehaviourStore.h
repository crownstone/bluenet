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
#include <common/cs_Component.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_ErrorCodes.h>

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
	inline std::array<Behaviour*, MaxBehaviours>& getActiveBehaviours() { return activeBehaviours; }

	/**
	 * Initialize store from flash.
	 */
	cs_ret_code_t init() override;

	virtual ~BehaviourStore();

	/**
	 * Add behaviour to the ActiveBehaviours if there is space.
	 * If null, nothing happens.
	 * This method takes ownership over the allocated resource
	 * (removeBehaviour and replaceBehaviour may delete it.)
	 */
	ErrorCodesGeneral addBehaviour(Behaviour* behaviour);

	/**
	 * Add behaviour to the ActiveBehaviours if there is space.
	 * If null, previous behaviour will be deleted.
	 * This method takes ownership over the allocated resource
	 * (removeBehaviour may delete it.)
	 */
	ErrorCodesGeneral replaceBehaviour(uint8_t index, Behaviour* behaviour);

	/**
	 * returns MaxBehaviours if not found.
	 */
	uint8_t FindEmptyIndex();


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
	ErrorCodesGeneral addBehaviour(uint8_t* buf, cs_buffer_size_t bufSize, uint8_t& index);

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
	/**
	 * call state store for the given switch type and update masterhash.
	 */
	void StoreUpdate(uint8_t index, SwitchBehaviour::Type type, uint8_t* buf, cs_buffer_size_t bufSize);
	/**
	 * call state store for the given behaviour and update masterhash.
	 */
	void StoreUpdate(uint8_t index, Behaviour* behaviour);

	/**
	 * Heap allocate an instance of given type from the buffer.
	 *
	 * Notes:
	 *  - If bufSize is less than the WireFormat::size for the given type, a default constructed behaviour
	 *    is allocated.
	 *  - The index is not checked for preexisting data. That must be deleted before using this function.
	 */
	Behaviour* allocateBehaviour(uint8_t index, SwitchBehaviour::Type type, uint8_t* buf, cs_buffer_size_t bufSize);

	/**
	 * Assign it to activateBehaviour[index] and print it.
	 */
	void assignBehaviour(uint8_t index, Behaviour* behaviour);

	/**
	 * If `type` is not a valid behaviour type: ERR_WRONG_PARAMETER
	 * If `bufSize` does not match expected size: ERR_WRONG_PAYLOAD_LENGTH
	 * Else: ERR_SUCCESS.
	 */
	ErrorCodesGeneral checkSizeAndType(SwitchBehaviour::Type type, cs_buffer_size_t bufSize);

	/**
	 * returns 0 for unknown types, else the obvious.
	 */
	size_t getBehaviourSize(SwitchBehaviour::Type type);

	void dispatchBehaviourMutationEvent();

	// checks intermediate state of handleReplaceBehaviour for consistency.
	ErrorCodesGeneral ReplaceParameterValidation(event_t& evt, uint8_t index, SwitchBehaviour::Type type);

	// loads the behaviours from state into the 'activeBehaviours' array.
	// BehaviourType must match BehaviourCsType.
	template <class BehaviourType>
	void LoadBehavioursFromMemory(CS_TYPE BehaviourCsType);
};
