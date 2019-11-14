/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventListener.h>
#include <presence/cs_PresenceDescription.h>
#include <time/cs_SystemTime.h>

#include <list>

/**
 * This handler listens for background advertisements to 
 * find out which users are in which room. It can be queried
 * by other 
 */
class PresenceHandler: public EventListener{
    private:
    // after this amount of seconds a presence_record becomes invalid.
    static const constexpr uint32_t presence_time_out = 60*5;

    // using a list because of constant time insertion/deletion of
    // any item in the container
    static const constexpr uint8_t max_records = 20;
    struct PresenceRecord {
        Time when;
        uint8_t who;
        uint8_t where;
    };
    static std::list<PresenceRecord> WhenWhoWhere;

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
    static PresenceStateDescription getCurrentPresenceDescription();
};