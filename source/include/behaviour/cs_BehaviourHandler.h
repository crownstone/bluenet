/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <behaviour/cs_BehaviourStore.h>
#include <behaviour/cs_SwitchBehaviour.h>
#include <common/cs_Component.h>
#include <events/cs_EventListener.h>
#include <presence/cs_PresenceHandler.h>

#include <optional>
#include <test/cs_TestAccess.h>

class BehaviourHandler : public EventListener, public Component {
    friend class TestAccess<BehaviourHandler>;
public:
	/**
	 * Obtains a pointer to presence handler, if it exists.
	 */
	virtual cs_ret_code_t init() override;

	/**
	 * Computes the intended behaviour state of this crownstone based on
	 * the stored behaviours, and then dispatches an event for that.
	 *
	 * Events:
	 * - EVT_PRESENCE_MUTATION
	 * - EVT_BEHAVIOURSTORE_MUTATION
	 * - STATE_BEHAVIOUR_SETTINGS
	 * - CMD_GET_BEHAVIOUR_DEBUG
	 */
	virtual void handleEvent(event_t& evt);

	/**
	 * Acquires the current time and presence information.
	 * Checks and updates the currentIntendedState by looping over the active behaviours.
	 *
	 * If isActive is false, or _presenceHandler is nullptr, this method has no effect.
	 *
	 * Returns true.
	 */
	bool update();

	/**
	 * Returns currentIntendedState variable and updates the previousIntendedState
	 * to currentIntendedState to match previousIntendedState.
	 */
	std::optional<uint8_t> getValue();

	/**
	 * Returns true if a behaviour at given time requires presence
	 */
	bool requiresPresence(Time t);

	/**
	 * Returns true if a behaviour at given time requires absence
	 */
	bool requiresAbsence(Time t);

private:
	/**
	 * Cached reference to the presence handler. (obtained at init)
	 */
	PresenceHandler* _presenceHandler                              = nullptr;

	/**
	 * cached reference to the behaviour store. (obtained at init)
	 */
	BehaviourStore* _behaviourStore                                = nullptr;

	/**
	 * The last value returned by getValue.
	 */
	std::optional<uint8_t> previousIntendedState                   = {};

	/**
	 *  The last value that was updated by the update method.
	 */
	std::optional<uint8_t> currentIntendedState                    = {};

	/**
	 * setting this to false will result in a BehaviourHandler that will
	 * not have an opinion about the state anymore (getValue returns std::nullopt).
	 */
	bool _isActive                                                 = true;

	/**
	 * Whether the behaviour settings are synced.
	 */
	bool _behaviourSettingsSynced                                  = false;

	/**
	 * Cache the received behaviour settings during syncing.
	 */
	std::optional<behaviour_settings_t> _receivedBehaviourSettings = {};

	// -----------------------------------------------------------------------
	// --------------------------- private methods ---------------------------
	// -----------------------------------------------------------------------

	/**
	 * Given current time/presence, query the behaviourstore and check
	 * if there any valid ones.
	 *
	 * Returns an empty optional when:
	 *  - this BehaviourHandler is inactive, or
	 *  - _presenceHandler is nullptr, or
	 *  - more than one valid behaviours contradicted each other.
	 *
	 * Returns a non-empty optional if a valid behaviour is found or
	 * multiple agreeing behaviours have been found.
	 * In this case its value contains the desired state value.
	 * When no behaviours are valid at given time/presence the intended
	 * value is 0. (house is 'off' by default)
	 */
	std::optional<uint8_t> computeIntendedState(Time currenttime, PresenceStateDescription currentpresence) const;

    /**
     * Returns most specific active switch behaviour, resolving conflicts. None if no behaviours are active.
     * Requires _behaviourStore to be non-null and currentTime.isValid() == true.
     * @param currentTime
     * @param currentPresence
     * @return
     */
    SwitchBehaviour* resolveSwitchBehaviour(Time currentTime, PresenceStateDescription currentPresence) const;

	void handleGetBehaviourDebug(event_t& evt);

	/**
	 * Returns b, casted as switch behaviour if that cast is valid and
	 * isValid(*) returns true.
	 * Else, returns nullptr.
	 */
	SwitchBehaviour* ValidateSwitchBehaviour(
			Behaviour* behave, Time currentTime, PresenceStateDescription currentPresence) const;

	// -----------------------------------------------------------------------
	// --------------------------- synchronization ---------------------------
	// -----------------------------------------------------------------------

	void onBehaviourSettingsMeshMsg(behaviour_settings_t settings);

	/**
	 * To be called when the behaviour settings were changed, and on sync response.
	 */
	void onBehaviourSettingsChange(behaviour_settings_t settings);

	/**
	 * Returns true when the behaviour settings should be synced.
	 * To be called when a sync request will be sent out.
	 */
	bool onBehaviourSettingsOutgoingSyncRequest();

	/**
	 * Finalizes the behaviour settings sync.
	 * To be called when the overall sync failed.
	 */
	void onMeshSyncFailed();

	/**
	 * If there was any behaviour settings sync response, the behaviour settings sync will be finalized.
	 */
	void tryFinalizeBehaviourSettingsSync();

	/**
	 * Sends a sync response.
	 * To be called on a behaviour settings sync request
	 */
	void onBehaviourSettingsIncomingSyncRequest();
};
