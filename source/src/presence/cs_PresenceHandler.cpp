/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <presence/cs_PresenceHandler.h>

#include <cs_Crownstone.h>
#include <time/cs_TimeOfDay.h>
#include <common/cs_Types.h>

#define LOGPresenceHandler(...) 

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

    adv_background_parsed_t* parsed_adv_ptr = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);

    if(WhenWhoWhere.size() >= max_records){
        WhenWhoWhere.pop_back();
    }

    uint32_t now = Crownstone::getTickCount();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out,now);

    if(parsed_adv_ptr->profileId == 0xff && parsed_adv_ptr->locationId == 0xff){
        LOGw("DEBUG: removing presence record data");
        WhenWhoWhere.clear();
    } else {        
        WhenWhoWhere.remove_if( 
            [&] (PresenceRecord www){ 
                if(!valid_time_interval.contains(www.when)){
                    LOGPresenceHandler("erasing old presence_record for user id %d because it's outdated", www.who);
                    return true;
                }
                if(www.who == parsed_adv_ptr->profileId){
                    LOGPresenceHandler("erasing old presence_record for user id %d because of new entry", www.who);
                    return true;
                }
                return false;
            } 
        );
        WhenWhoWhere.push_front( {now, parsed_adv_ptr->profileId, parsed_adv_ptr->locationId} );
    }

    event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION,nullptr,0);
    presence_event.dispatch();

    // TODO: extract handling into method and clean up.
}

void PresenceHandler::removeOldRecords(){
    uint32_t now = Crownstone::getTickCount();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out,now);

     WhenWhoWhere.remove_if( 
        [&] (PresenceRecord www){ 
            if(!valid_time_interval.contains(www.when)){
                LOGPresenceHandler("erasing old presence_record for user id %d because it's outdated", www.who);
                return true;
            }
           
            return false;
        }
    );
}

PresenceStateDescription PresenceHandler::getCurrentPresenceDescription(){
    PresenceStateDescription p = 0;
    uint32_t now = Crownstone::getTickCount();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out,now);

    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ){
        if(!valid_time_interval.contains(iter->when)){
            LOGPresenceHandler("erasing old presence_record for user id %d because it's already %d ticks old", 
                iter->who, now - iter->when
            );
            iter = WhenWhoWhere.erase(iter); // increments iter and invalidates previous value..
            continue;
        } else {
            // appearently iter is valid, so the .where field describes an occupied room.
            p |= 1 << CsMath::min(64-1,iter->where);
            ++iter;
        }
    }
    return p;
}

void PresenceHandler::print(){
    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); iter++){
        LOGd("at %d ticks after startup user #%d was found in room %d", iter->when, iter->who, iter->where);
    }
}