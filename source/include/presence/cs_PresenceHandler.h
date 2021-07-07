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
    enum class MutationType : uint8_t {
        NothingChanged              ,
        Online                      , // when no previous PresenceStateDescription was available but now it is
        Offline                     , // when a previous PresenceStateDescription was available but now it isn't
        LastUserExitSphere          , 
        FirstUserEnterSphere        , 
        OccupiedRoomsMaskChanged    ,
    };

    static const constexpr uint8_t max_location_id = 63;
    static const constexpr uint8_t max_profile_id = 7;

private:
    // after this amount of seconds a presence_record becomes invalid.
    static const constexpr uint8_t presence_time_out_s = 10;

    // For each presence entry, send it max every (x + variation) seconds over the mesh.
    static const constexpr uint8_t presence_mesh_send_throttle_seconds = 10;
    static const constexpr uint8_t presence_mesh_send_throttle_seconds_variation = 20;

    /**
     * after this amount of seconds it is assumed that presencehandler would have received 
     * message from all devices in vicinity of this device.
     */
    static const constexpr uint32_t presence_uncertain_due_reboot_time_out_s = 30;


    // using a list because of constant time insertion/deletion of
    // any item in the container
    static const constexpr uint8_t max_records = 20;
    struct PresenceRecord {
        uint8_t who;    // profile id
        uint8_t where;  // room id
        /**
         * Used to determine when a record is timed out.
         * Decreases every seconds.
         * Starts at presence_time_out_s, when 0, it is timed out.
         */
    	uint8_t timeoutCountdownSeconds;
    	/**
    	 * Used to determine whether to send a mesh message.
    	 * Decreases every seconds.
    	 * Starts at presence_mesh_send_throttle_seconds, when 0, a mesh message can be sent.
    	 */
    	uint8_t meshSendCountdownSeconds;
        PresenceRecord(
        		uint8_t profileId,
				uint8_t roomId,
				uint8_t timeoutSeconds = presence_time_out_s,
				uint8_t meshThrottleSeconds = presence_mesh_send_throttle_seconds):
        	who(profileId),
			where(roomId),
			timeoutCountdownSeconds(timeoutSeconds),
			meshSendCountdownSeconds(meshThrottleSeconds)
        {}
    };

    /**
     * keeps track of a short history of presence events.
     * will be lazily updated to remove old entries: 
     *  - when new presence is detected
     *  - when getCurrentPresenceDescription() is called
     */
    static std::list<PresenceRecord> WhenWhoWhere;

    /**
     * Clears the WhenWhoWhere list from entries that have a time stamp older than
     * presence_time_out_s.
     */
    void removeOldRecords();

    /**
     * Calls handleProfileLocationAdministration, and dispatches events based
     * on the returned mutation type.
     */
	void handlePresenceEvent(uint8_t profile, uint8_t location, bool fromMesh);

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

    // out of order
    void print();
    
    TYPIFY(CONFIG_CROWNSTONE_ID) _ownId = 0;

public:
    // register as event handler
    void init();

    /**
     * receive background messages indicating where users are,
     * record the time and place and update the current presence description
     * when necessary
     */
    virtual void handleEvent(event_t& evt) override;

    /**
     * Returns a simplified description of the current presence knowledge,
     * each bit in the description indicates if a person is in that room
     * or not.
     */
    static std::optional<PresenceStateDescription> getCurrentPresenceDescription();

};
