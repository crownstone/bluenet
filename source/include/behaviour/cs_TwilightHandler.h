/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>

#include <behaviour/cs_BehaviourStore.h>
#include <presence/cs_PresenceDescription.h>
#include <time/cs_Time.h>

#include <optional>

class TwilightHandler : public EventListener, public Component {
public:
	/**
	 * Initialize this class:
	 * - Read settings from flash.
	 * - Start listening for events.
	 * - obtain reference to behaviourstore
	 */
	cs_ret_code_t init() override;

	/**
	 * Computes the twilight state of this crownstone based on
	 * the stored behaviours, and then dispatches an event.
	 *
	 * Events:
	 * - EVT_PRESENCE_MUTATION
	 * - EVT_BEHAVIOURSTORE_MUTATION
	 * - STATE_BEHAVIOUR_SETTINGS
	 */
	void handleEvent(event_t& evt) override;

	/**
	 * Acquires the current time and presence information.
	 * Checks the intended state by looping over the active behaviours
	 * and if the intendedState differs from previousIntendedState
	 * dispatch an event to communicate a state update.
	 *
	 * if time is not valid, aborts method execution and returns false.
	 * returns true when value was updated, false else.
	 */
	bool update();

	/**
	 * Returns currentIntendedState.
	 */
	std::optional<uint8_t> getValue();

private:
	/**
	 * Given current time, query the behaviourstore and check
	 * if there any valid ones.
	 *
	 * Returns an empty optional if time is invalid, or this isActive==false.
	 * Else returns a non-empty optional containing the conflict resolved value
	 * of all active twilights, defaulting to 100 if none are active.
	 */
	std::optional<uint8_t> computeIntendedState(Time currentTime);

	std::optional<uint8_t> currentIntendedState = 100;

	/**
	 * Is this handler active?
	 */
	bool isActive = true;

	/**
	 * cached reference to the behaviour store. (obtained at init)
	 */
	BehaviourStore* _behaviourStore = nullptr;
};
