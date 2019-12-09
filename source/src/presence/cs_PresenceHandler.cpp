/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <presence/cs_PresenceHandler.h>

#include <time/cs_TimeOfDay.h>
#include <common/cs_Types.h>

#include <drivers/cs_Serial.h>

#define LOGPresenceHandler(...) LOGnone(__VA_ARGS__)

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

    // TODO inputvalidation

    uint32_t now = SystemTime::up();
    auto valid_time_interval = CsMath::Interval(now-presence_time_out_s,presence_time_out_s);

    if(parsed_adv_ptr->profileId == 0xff && parsed_adv_ptr->locationId == 0xff){
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
                if(www.who == parsed_adv_ptr->profileId){
                    LOGPresenceHandler("erasing old presence_record for user id %d because of new entry", www.who);
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