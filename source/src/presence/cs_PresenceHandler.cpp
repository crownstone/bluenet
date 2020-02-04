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

#define LOGPresenceHandler LOGnone

//#define PRESENCE_HANDLER_TESTING_CODE

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
    bool fromMesh = false;
    switch(evt.type) {
    case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
        // drop through
        adv_background_parsed_t* parsed_adv_ptr = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);
        if (BLEutil::isBitSet(parsed_adv_ptr->flags, BG_ADV_FLAG_IGNORE_FOR_PRESENCE)) {
        	return;
        }
        profile = parsed_adv_ptr->profileId;
        location = parsed_adv_ptr->locationId;
        break;
    }
    case CS_TYPE::EVT_MESH_PROFILE_LOCATION: {
        TYPIFY(EVT_MESH_PROFILE_LOCATION) *profile_location = (TYPIFY(EVT_MESH_PROFILE_LOCATION)*)evt.data;
        profile = profile_location->profile;
        location = profile_location->location;
        fromMesh = true;
        LOGPresenceHandler("From mesh: location=%u profile=%u", profile, location);
		break;
    }
    case CS_TYPE::EVT_TICK: {
    	TYPIFY(EVT_TICK)* tickCount = reinterpret_cast<TYPIFY(EVT_TICK)*>(evt.data);
    	if (*tickCount % (1000 / TICK_INTERVAL_MS) == 0) {
    		tickSecond();
    	}
    	return;
    }
    default:
        return;
    }
    // Validate data.
    if (profile > max_profile_id || location > max_location_id) {
    	LOGw("Invalid profile(%u) or location(%u)", profile, location);
    	return;
    }
    MutationType mutation = handleProfileLocationAdministration(profile, location, fromMesh);
    if (mutation != MutationType::NothingChanged) {
    	triggerPresenceMutation(mutation);
    }
}

PresenceHandler::MutationType PresenceHandler::handleProfileLocationAdministration(uint8_t profile, uint8_t location, bool fromMesh) {
    auto prevdescription = getCurrentPresenceDescription();

#ifdef PRESENCE_HANDLER_TESTING_CODE
    if(profile == 0xff && location == 0xff){
        LOGw("DEBUG: clear presence record data");
        
        if(prevdescription.value_or(0) != 0){
            // sphere exit
            WhenWhoWhere.clear();
            return MutationType::LastUserExitSphere;
        }

        return MutationType::NothingChanged;
    }
#endif

    // purge whowhenwhere of old entries and add a new entry
    if (WhenWhoWhere.size() >= max_records) {
    	LOGw("Reached max number of records");
        WhenWhoWhere.pop_back();
    }
    uint8_t meshCountdown = 0;
    for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end();) {
		if (iter->timeoutCountdownSeconds == 0) {
			// Should not happen, record should've been removed.
			LOGw("timed out record");
		}
    	if (iter->who == profile && iter->where == location) {
    		LOGPresenceHandler("erasing old record profile(%u) location(%u)", profile, location);
    		meshCountdown = iter->meshSendCountdownSeconds;
    		WhenWhoWhere.erase(iter);
    		break;
    	}
    	++iter;
    }
    // When record is new, or the old record mesh send countdown was 0: send profile location over the mesh.
    if (meshCountdown == 0) {
    	if (!fromMesh) {
    		propagateMeshMessage(profile, location);
    	}
    	meshCountdown = presence_mesh_send_throttle_seconds;
    }
    LOGPresenceHandler("add record profile(%u) location(%u)", profile, location);
    WhenWhoWhere.push_front(PresenceRecord(profile, location, presence_time_out_s, meshCountdown));

    auto nextdescription = getCurrentPresenceDescription();
    return getMutationType(prevdescription,nextdescription);
}

PresenceHandler::MutationType PresenceHandler::getMutationType(
        std::optional<PresenceStateDescription> prevdescription, 
        std::optional<PresenceStateDescription> nextdescription) {

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

void PresenceHandler::removeOldRecords() {
     WhenWhoWhere.remove_if( 
        [&] (PresenceRecord www) {
			if (www.timeoutCountdownSeconds == 0) {
				LOGPresenceHandler("erasing timed out record profile=%u location=%u", www.who, www.where);
				return true;
			}
			return false;
        }
    );
}

void PresenceHandler::triggerPresenceMutation(MutationType mutationtype){
    event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION,&mutationtype,sizeof(mutationtype));
    presence_event.dispatch();
}

void PresenceHandler::propagateMeshMessage(uint8_t profile, uint8_t location){
    TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION) eventData;
    eventData.profile = profile;
    eventData.location = location;
    event_t event(CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION, &eventData, sizeof(eventData));
    event.dispatch();
}

std::optional<PresenceStateDescription> PresenceHandler::getCurrentPresenceDescription() {
    if (SystemTime::up() < presence_uncertain_due_reboot_time_out_s) {
        LOGPresenceHandler("presence_uncertain_due_reboot_time_out_s hasn't expired");
        return {};
    }
    PresenceStateDescription p;
    for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ++iter) {
    	if (iter->timeoutCountdownSeconds == 0) {
    		// Should not happen, record should've been removed.
    		LOGw("timed out record");
    	}
    	else {
    		// appearently iter is valid, so the .where field describes an occupied room.
			p.setRoom(iter->where);
			// LOGPresenceHandler("adding room %d to currentPresenceDescription", iter->where);
    	}
    }
    return p;
}

void PresenceHandler::tickSecond() {
	for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end();) {
		if (iter->timeoutCountdownSeconds) {
			iter->timeoutCountdownSeconds--;
		}
		if (iter->timeoutCountdownSeconds == 0) {
			iter = WhenWhoWhere.erase(iter);
			// 8-1-2020 TODO Bart: send event?
		}
		else {
			if (iter->meshSendCountdownSeconds) {
				iter->meshSendCountdownSeconds--;
			}
			++iter;
		}
	}
}

void PresenceHandler::print(){
    // for(auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); iter++){
    //     LOGd("at %d seconds after startup user #%d was found in room %d", iter->when, iter->who, iter->where);
    // }
    
    std::optional<PresenceStateDescription> desc = getCurrentPresenceDescription();
    if(desc){
        // desc->print();
    } else {
        LOGPresenceHandler("presenchandler status: unavailable");
    }
}
