/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <optional>
#include <presence/cs_PresenceDescription.h>
#include <time/cs_SystemTime.h>
#include <util/cs_Store.h>

/**
 * Keeps up all the locations each profile is present in.
 * Sends out event when this changes.
 * Sends out throttled mesh messages when the location of a profile is received.
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


private:
    /** Number of seconds before presence times out. */
    static const constexpr uint8_t PRESENCE_TIMEOUT_SECONDS = 10;

    /** For each presence entry, send it max every (x + variation) seconds over the mesh. */
    static const constexpr uint8_t PRESENCE_MESH_SEND_THROTTLE_SECONDS = 10;
    static const constexpr uint8_t PRESENCE_MESH_SEND_THROTTLE_SECONDS_VARIATION = 20;

    /**
     * Number of seconds after boot it is assumed to take to receive the location of all devices.
     */
    static const constexpr uint32_t PRESENCE_UNCERTAIN_SECONDS_AFTER_BOOT = 30;

    /**
     * Maximum number of presence records that is kept up.
     *
     * Must be smaller than 0xFF.
     */
    static const constexpr uint8_t MAX_RECORDS = 20;

    struct __attribute__((__packed__)) profile_location_t {
		static const constexpr uint8_t MAX_LOCATION_ID = 63;
		static const constexpr uint8_t MAX_PROFILE_ID = 7;

    	uint8_t profile;
		uint8_t location;

		bool operator==(const profile_location_t& other) {
			return memcmp(this, &other, sizeof(*this)) == 0;
		}

		bool isValid() {
			return profile <= MAX_PROFILE_ID && location <= MAX_LOCATION_ID;
		}
    };

    struct PresenceRecord {
    	profile_location_t profileLocation;
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
						profile_location_t profileLocation = {},
						uint8_t timeoutSeconds      = PRESENCE_TIMEOUT_SECONDS,
						uint8_t meshThrottleSeconds = 0)
					: profileLocation(profileLocation)
					, timeoutCountdownSeconds(timeoutSeconds)
					, meshSendCountdownSeconds(meshThrottleSeconds) {}

		void invalidate() {
        	timeoutCountdownSeconds = 0;
        }

        bool isValid() {
        	return timeoutCountdownSeconds != 0;
        }

        profile_location_t id() { return profileLocation; }
    };

    /**
     * Stores presence records.
     */
    static Store<PresenceRecord, MAX_RECORDS> _store;

	/**
	 * finds oldest record and default constructs its present record,
	 * then returns the pointer to it.
	 */
	PresenceRecord* clearOldestRecord(profile_location_t profileLocation);

    /**
     * Handle an incoming profile-location combination.
     * - Validates profile and location.
     * - Calls handleProfileLocation().
     * - Dispatches events based on the returned mutation type.
     */
	void handlePresenceEvent(profile_location_t profileLocation, bool forwardToMesh);

	/**
     * Handle an incoming profile-location combination.
     * - Stores the profile-location.
     * - Sends a mesh message.
     * - Dispatches change event.
     *
     * @param[in] profile         The profile ID.
     * @param[in] location        The location ID.
     * @param[in] forwardToMesh   If true, the update will be pushed into the mesh (throttled).
     * @return                    Mutation type.
     */
    PresenceMutation handleProfileLocation(profile_location_t profileLocation, bool forwardToMesh);

    /**
     * Resolves the type of mutation from previous and next descriptions.
     */
    static PresenceMutation getMutationType(
        std::optional<PresenceStateDescription> prevDescription, 
        std::optional<PresenceStateDescription> nextDescription);

    /**
     * Send a mesh message with profile and location.
     */
    void sendMeshMessage(profile_location_t profileLocation);

    /**
     * Sends presence change event.
     *
     * @param[in] type                 Type of change.
     * @param[in] profileLocation      The relevant profile ID and location.
     */
    void dispatchPresenceChangeEvent(PresenceChange type, profile_location_t profileLocation = {});

    /**
     * Sends presence mutation event.
     * This event is deprecated, but still used.
     */
    void dispatchPresenceMutationEvent(PresenceMutation mutation);

    /**
     * To be called every second. Decreases timeoutCountdownSeconds of the records
     * and dispatches exit-events when necessary.
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
