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

#define LOGPresenceHandler LOGd

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

    MutationType mt = handleProfileLocationAdministration(profile,location);

    LOGd("MutationEvent: %d",static_cast<uint8_t>(mt));
    print();
    
    if(mt != MutationType::NothingChanged){
        triggerPresenseMutation(mt);
    }

    propagateMeshMessage(profile,location);
}

PresenceHandler::MutationType PresenceHandler::handleProfileLocationAdministration(uint8_t profile, uint8_t location){

    // TODO inputvalidation
    auto prevdescription = getCurrentPresenceDescription();
    LOGd("prev description:");
    print();

    if(profile == 0xff && location == 0xff){
        LOGw("DEBUG: clear presence record data");
        
        if(prevdescription.value_or(0) != 0){
            // sphere exit
            WhenWhoWhere.clear();
            return MutationType::LastUserExitSphere;
        }

        return MutationType::NothingChanged;
    }

    uint32_t now = SystemTime::up();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out_s,presence_time_out_s);
    
    // purge whowhenwhere of old entries and add a new entry
    if(WhenWhoWhere.size() >= max_records){
        WhenWhoWhere.pop_back();
    }

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

    auto nextdescription = getCurrentPresenceDescription();
    LOGd("next description:");
    print();

    if(prevdescription == nextdescription){
        return MutationType::NothingChanged; // neither has_value or value()'s eq.
    }
    if(prevdescription.has_value() && nextdescription.has_value()){
        // values unequal so can distinguish on == 0 of a single value here
        if(*prevdescription == 0){
            return MutationType::FirstUserEnterSphere;
        }
        if(*nextdescription == 0){
            return MutationType::LastUserExitSphere;
        }
        // both non-zero, non-trivial change happened.
        return MutationType::OccupiedRoomsMaskChanged;
    }
    if(nextdescription.has_value()){
        return MutationType::Online;
    }
    if(prevdescription.has_value()){
        return MutationType::Offline;
    }

    // can't reach here but that's too difficult for the compiler to conclude.
    LOGw("unhandled presence handler description");
    return MutationType::NothingChanged;
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

void PresenceHandler::triggerPresenseMutation(MutationType mutationtype){
    event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION,&mutationtype,sizeof(mutationtype));
    presence_event.dispatch();
}

void PresenceHandler::propagateMeshMessage(uint8_t profile, uint8_t location){
    TYPIFY(EVT_PROFILE_LOCATION) profile_location;
    profile_location.profile = profile;
    profile_location.location = location;
    profile_location.stone_id = _ownId;

    event_t profile_location_event(CS_TYPE::EVT_PROFILE_LOCATION, &profile_location, sizeof(profile_location));
    profile_location_event.dispatch();
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
            // LOGPresenceHandler("adding room %d to currentPresenceDescription", iter->where);
            ++iter;
        }
    }

    return p;
}

void PresenceHandler::print(){
    // for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); iter++){
    //     LOGd("at %d seconds after startup user #%d was found in room %d", iter->when, iter->who, iter->where);
    // }
    
    std::optional<PresenceStateDescription> desc = getCurrentPresenceDescription();
    if( desc){
        uint32_t rooms[2] = {
            static_cast<uint32_t>(*desc >> 0 ),
            static_cast<uint32_t>(*desc >> 32)
        };
        LOGd("presenchandler status: %x %x" , rooms[1], rooms[0]);
    } else {
        LOGd("presenchandler status: unavailable");
    }
}
