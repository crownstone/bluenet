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
class PresenceHandler: public EventListener{
    private:
    // after this amount of seconds a presence_record becomes invalid.
    static const constexpr uint32_t presence_time_out_s = 5*60;

    // after this amount of seconds it is assumed that presencehandler would have received 
    // a message from all devices in vicinity of this device.
    static const constexpr uint32_t presence_uncertain_due_reboot_time_out_s = 30;

    // using a list because of constant time insertion/deletion of
    // any item in the container
    static const constexpr uint8_t max_records = 20;
    struct PresenceRecord {
        uint32_t when;  // ticks since startup
        uint8_t who;    // profile id
        uint8_t where;  // room id
    };

    // keeps track of a short history of presence events.
    // will be lazily updated to remove old entries: 
    //  - when new presence is detected
    //  - when getCurrentPresenceDescription() is called
    static std::list<PresenceRecord> WhenWhoWhere;

    void removeOldRecords();
    void print();

    public:
    // receive background messages indicating where users are,
    // record the time and place and update the current presence description
    // when necessary
    virtual void handleEvent(event_t& evt) override;

    /**
     * Returns a simplified description of the current presence knowledge,
     * each bit in the description indicates if a person is in that room
     * or not.
     */
    static std::optional<PresenceStateDescription> getCurrentPresenceDescription();
};