/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <presence/cs_PresenceHandler.h>

#include <time/cs_TimeOfDay.h>
#include <common/cs_Types.h>

#include <storage/cs_State.h>

#include <drivers/cs_Serial.h>

#define LOGPresenceHandler(...) LOGnone(__VA_ARGS__)

std::list<PresenceHandler::PresenceRecord> PresenceHandler::WhenWhoWhere;

void PresenceHandler::init() {
    State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownId, sizeof(_ownId));

    // TODO Anne @Arend. The listener is now set in cs_Crownstone.cpp, outside of the class. This seems to be an 
    // implementation detail however that should be part of the class. If you want the user to start and stop
    // listening to events, I'd add member functions for those.

    // EventDispatcher::getInstance().addListener(this);
}

void PresenceHandler::handleEvent(event_t& evt){

    uint8_t location;
    uint8_t profile;

    switch(evt.type) {
    case CS_TYPE::STATE_TIME: {
        // TODO: 
        // - when time moves backwards because of daylight saving time, we can adjust the values
        //   in the list in order to keep the data intact.
        // - when time moves forwards by an hour, the same should be done.
        // - when prevtime (which is part of the event data) is invalid, or a strange difference has
        //   occured the records should be completely purged.
        removeOldRecords();
        return;
    }
    case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
        // drop through
        adv_background_parsed_t* parsed_adv_ptr = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);
        profile = parsed_adv_ptr->profileId;
        location = parsed_adv_ptr->locationId;
		//LOGd("Location [phone]: %x %x", profile, location);
        break;
    }
    case CS_TYPE::EVT_PROFILE_LOCATION: {
        // filter on own messages
        // TODO Anne @Bart This is probably a common theme. Why not incorporate it in the EventDispatcher itself? Also
        // we do not care if this stone "has already an id", we actually want to just ignore messages from this 
        // particular module. An id per "broadcaster" would be sufficient.
        TYPIFY(EVT_PROFILE_LOCATION) *profile_location = (TYPIFY(EVT_PROFILE_LOCATION)*)evt.data;
        if (profile_location->stone_id == _ownId) {
            return;
        }
        profile = profile_location->profile;
        location = profile_location->location;
		//LOGd("Location [mesh]: %x %x", profile, location);
		break;
    }
    default:
        return;
    }

    if(WhenWhoWhere.size() >= max_records){
        WhenWhoWhere.pop_back();
    }

    // TODO inputvalidation

    uint32_t now = SystemTime::up();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out_s,presence_time_out_s);

    if(profile == 0xff && location == 0xff){
        LOGw("DEBUG: removing presence record data");
        WhenWhoWhere.clear();
    } else {        
        WhenWhoWhere.remove_if( 
            [&] (PresenceRecord www){ 
                if(!valid_time_interval.ClosureContains(www.when)){
                    LOGPresenceHandler("erasing old presence_record for user id %d because it's outdated: %d not contained in [%d %d]", 
                        www.who,www.when,valid_time_interval.lowerbound(),valid_time_interval.upperbound());
                    return true;
                }
                if(www.who == profile){
                    LOGPresenceHandler("erasing old presence_record for user id %d because of new entry", www.who);
                    return true;
                }
                return false;
            } 
        );
        WhenWhoWhere.push_front( {now, profile, location} );
    }

    // TODO Anne @Arend: should we not bail out when profileId == 0xff, why proceed with presence mutation event?

    print();

    event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION,nullptr,0);
    presence_event.dispatch();

    TYPIFY(EVT_PROFILE_LOCATION) profile_location;
    profile_location.profile = profile;
    profile_location.location = location;
    profile_location.stone_id = _ownId;

    event_t profile_location_event(CS_TYPE::EVT_PROFILE_LOCATION, &profile_location, sizeof(profile_location));
    profile_location_event.dispatch();

    // TODO: extract handling into method and clean up.
}

void PresenceHandler::removeOldRecords(){
    uint32_t now = SystemTime::up();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out_s,presence_time_out_s);

     WhenWhoWhere.remove_if( 
        [&] (PresenceRecord www){ 
            if(!valid_time_interval.ClosureContains(www.when)){
                LOGPresenceHandler("erasing old presence_record for user id %d because it's outdated: %d not contained in [%d %d]", 
                        www.who,www.when,valid_time_interval.lowerbound(),valid_time_interval.upperbound());
                return true;
            }
           
            return false;
        }
    );
}

std::optional<PresenceStateDescription> PresenceHandler::getCurrentPresenceDescription(){
    if(SystemTime::up() < presence_uncertain_due_reboot_time_out_s){
        LOGPresenceHandler("presence_uncertain_due_reboot_time_out_s hasn't expired");
        return {};
    }

    PresenceStateDescription p = 0;
    uint32_t now = SystemTime::up();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out_s,presence_time_out_s);

    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ){
        if(!valid_time_interval.ClosureContains(iter->when)){
            LOGPresenceHandler("erasing old presence_record for user id %d because it's outdated: %d not contained in [%d %d]", 
                        iter->who,iter->when,valid_time_interval.lowerbound(),valid_time_interval.upperbound());

            iter = WhenWhoWhere.erase(iter); // increments iter and invalidates previous value..
            continue;
        } else {
            // appearently iter is valid, so the .where field describes an occupied room.
            p |= 1 << CsMath::min(64-1,iter->where);
            LOGPresenceHandler("adding room %d to currentPresenceDescription", iter->where);
            ++iter;
        }
    }

    return p;
}

void PresenceHandler::print(){
    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); iter++){
        LOGd("at %d seconds after startup user #%d was found in room %d", iter->when, iter->who, iter->where);
    }
}
