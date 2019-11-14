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


void PresenceHandler::handleEvent(event_t& evt){
    if(evt.type != CS_TYPE::EVT_ADV_BACKGROUND_PARSED){
        return;
    }
    
    adv_background_parsed_t* parsed_adv_ptr = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);

    if(WhenWhoWhere.size() >= max_records){
        WhenWhoWhere.pop_back();
    }

    Time now = SystemTime::posix();
    uint32_t time_out_threshold = now - presence_time_out;

    WhenWhoWhere.remove_if( 
        [&] (PresenceRecord www){ 
            return www.who == parsed_adv_ptr->profileId || www.when < time_out_threshold;
        } 
    );
    WhenWhoWhere.push_front( {now, parsed_adv_ptr->profileId, parsed_adv_ptr->locationId} );

    print();
}

PresenceStateDescription PresenceHandler::getCurrentPresenceDescription(){
    PresenceStateDescription p = 0;
    for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ){
        if(iter->when < SystemTime::posix() - presence_time_out){
            iter = WhenWhoWhere.erase(iter); // increments iter and invalidates previous value..
            continue;
        } else {
            p |= 1 << CsMath::max(64,iter->where);
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