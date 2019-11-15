/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <time/cs_TimeOfDay.h>
#include <time/cs_SystemTime.h>
#include <presence/cs_PresenceHandler.h>
#include <common/cs_Types.h>

std::list<PresenceHandler::PresenceRecord> PresenceHandler::WhenWhoWhere;

void PresenceHandler::handleEvent(event_t& evt){
    if(evt.type == CS_TYPE::STATE_TIME){
        // TODO: 
        // - when time moves backwards because of daylight saving time, we can adjust the values
        //   in the list in order to keep the data intact.
        // - when time moves forwards by an hour, the same should be done.
        // - when prevtime (which is part of the event data) is invalid, or a strange difference has
        //   occured the records should be completely purged.
        removeOldRecords();
        return;
    }
    if(evt.type != CS_TYPE::EVT_ADV_BACKGROUND_PARSED){
        return;
    }
    
    LOGd("presence handler got adv parsed event");

    adv_background_parsed_t* parsed_adv_ptr = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);

    if(WhenWhoWhere.size() >= max_records){
        WhenWhoWhere.pop_back();
    }

    Time now = SystemTime::posix();
    uint32_t time_out_threshold = now - presence_time_out;

    if(parsed_adv_ptr->profileId == 0xff && parsed_adv_ptr->locationId == 0xff){
        LOGw("DEBUG: removing presence record data");
        WhenWhoWhere.clear();
    } else {
        WhenWhoWhere.remove_if( 
            [&] (PresenceRecord www){ 
                if(www.when < time_out_threshold){
                    LOGd("erasing old presence_record for user id %d because it's outdated", www.who);
                    return true;
                }
                if(www.who == parsed_adv_ptr->profileId){
                    LOGd("erasing old presence_record for user id %d because of new entry", www.who);
                    return true;
                }
                return false;
            } 
        );
        WhenWhoWhere.push_front( {now, parsed_adv_ptr->profileId, parsed_adv_ptr->locationId} );
    }

    print();

    event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION,nullptr,0);
    presence_event.dispatch();

    // TODO: extract handling into method and clean up.
}

void PresenceHandler::removeOldRecords(){
    Time now = SystemTime::posix();
    uint32_t time_out_threshold = now - presence_time_out;

     WhenWhoWhere.remove_if( 
        [&] (PresenceRecord www){ 
            if(www.when < time_out_threshold){
                LOGd("erasing old presence_record for user id %d because it's outdated", www.who);
                return true;
            }
           
            return false;
        } 
    );
}

PresenceStateDescription PresenceHandler::getCurrentPresenceDescription(){
    PresenceStateDescription p = 0;
    Time now = SystemTime::posix();
    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ){
        if(iter->when < now - presence_time_out){
            LOGd("erasing old presence_record for user id %d because it's already %d sec old", 
                iter->who, now - iter->when
            );
            iter = WhenWhoWhere.erase(iter); // increments iter and invalidates previous value..
            continue;
        } else {
            p |= 1 << CsMath::min(64-1,iter->where);
            ++iter;
        }
    }
    return p;
}

void PresenceHandler::print(){
    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); iter++){
        TimeOfDay t = iter->when;
        LOGd("at %02d:%02d:%02d user #%d was found in room %d", t.h(),t.m(),t.s(), iter->who, iter->where);
    }
}