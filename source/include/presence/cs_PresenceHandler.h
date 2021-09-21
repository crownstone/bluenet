/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <presence/cs_PresenceDescription.h>
#include <time/cs_SystemTime.h>

#include <list>
#include <optional>

/**
 * This handler listens for background advertisements to 
 * find out which users are in which room. It can be queried
 * by other 
 */
class PresenceHandler: public EventListener {
public:
	PresenceHandler();

    void init();

    /**
     * Returns a simplified description of the current presence knowledge,
     * each bit in the description indicates if a person is in that room
     * or not.
     */
    static std::optional<PresenceStateDescription> getCurrentPresenceDescription();


    enum class MutationType : uint8_t {
        NothingChanged              ,
        Online                      , // when no previous PresenceStateDescription was available but now it is
        Offline                     , // when a previous PresenceStateDescription was available but now it isn't
        LastUserExitSphere          , 
        FirstUserEnterSphere        , 
        OccupiedRoomsMaskChanged    ,
    };

    static const constexpr uint8_t MAX_LOCATION_ID = 63;
    static const constexpr uint8_t MAX_PROFILE_ID = 7;

private:
    // after this amount of seconds a presence_record becomes invalid.
    static const constexpr uint8_t PRESENCE_TIMEOUT_SECONDS = 10;

    // For each presence entry, send it max every (x + variation) seconds over the mesh.
    static const constexpr uint8_t PRESENCE_MESH_SEND_THROTTLE_SECONDS = 10;
    static const constexpr uint8_t PRESENCE_MESH_SEND_THROTTLE_SECONDS_VARIATION = 20;

    /**
     * after this amount of seconds it is assumed that presencehandler would have received 
     * message from all devices in vicinity of this device.
     */
    static const constexpr uint32_t PRESENCE_UNCERTAIN_SECONDS_AFTER_BOOT = 30;

    static const constexpr uint8_t MAX_RECORDS = 20;

    struct PresenceRecord {
        uint8_t profile;
        uint8_t location;
        /**
         * Used to determine when a record is timed out.
         * Decreases every seconds.
         * Starts at presence_time_out_s, when 0, it is timed out.
         */
    	uint8_t timeoutCountdownSeconds = 0;
    	/**
    	 * Used to determine whether to send a mesh message.
    	 * Decreases every seconds.
    	 * Starts at presence_mesh_send_throttle_seconds, when 0, a mesh message can be sent.
    	 */
    	uint8_t meshSendCountdownSeconds;
    	PresenceRecord() {}
    	PresenceRecord(
    			uint8_t profileId,
    			uint8_t roomId,
    			uint8_t timeoutSeconds = PRESENCE_TIMEOUT_SECONDS,
    			uint8_t meshThrottleSeconds = 0):
    				profile(profileId),
    				location(roomId),
    				timeoutCountdownSeconds(timeoutSeconds),
    				meshSendCountdownSeconds(meshThrottleSeconds)
    	{}

        void invalidate() {
        	timeoutCountdownSeconds = 0;
        }

        bool isValid() {
        	return timeoutCountdownSeconds != 0;
        }
    };

    /**
     * keeps track of a short history of presence events.
     * will be lazily updated to remove old entries:
     *  - when new presence is detected
     *  - when getCurrentPresenceDescription() is called
     */
    static PresenceRecord _presenceRecords[MAX_RECORDS];

    /**
     * Invalidate all presence records.
     */
    void resetRecords();

    /**
     * Find a record with given profile and location.
     * If not found, create a new one.
     * Returns a null pointer if there is no space for a new record.
     */
    PresenceRecord* findOrAddRecord(uint8_t profile, uint8_t location);

    /**
     * Calls handleProfileLocationAdministration, and dispatches events based
     * on the returned mutation type.
     */
	void handlePresenceEvent(uint8_t profile, uint8_t location, bool forwardToMesh);

	/**
     * Processes a new profile-location combination:
     * - a new entry is placed in the WhenWhoWhere list, 
     * - previous entries with the same p-l combo are deleted
     * - the WhenWhoWhere list is purged of old entries
     * @param[in] forwardToMesh If true, the update will be pushed into the mesh (throttled).
     */
    MutationType handleProfileLocationAdministration(uint8_t profile, uint8_t location, bool forwardToMesh);

    /**
     * Resolves the type of mutation from previous and next descriptions.
     */
    static MutationType getMutationType(
        std::optional<PresenceStateDescription> prevdescription, 
        std::optional<PresenceStateDescription> nextdescription);

    /**
     * Triggers a EVT_PROFILE_LOCATION event matching the given parameters.
     */
    void propagateMeshMessage(uint8_t profile, uint8_t location);

    /**
     * Sends event with profile location change.
     *
     * @param[in] type                 Type of change.
     * @param[in] profileId            The profile ID that entered/left a location.
     * @param[in] locationId           The location ID that was entered/left.
     */
    void sendPresenceChange(PresenceChange type, uint8_t profileId = 0, uint8_t locationId = 0);

    /**
     * Triggers a EVT_PRESENCE_MUTATION event of the given type.
     */
    void triggerPresenceMutation(MutationType mutationtype);

    /**
     * To be called every second.
     */
    void tickSecond();

public:
    /**
     * receive background messages indicating where users are,
     * record the time and place and update the current presence description
     * when necessary
     */
    virtual void handleEvent(event_t& evt) override;

};
