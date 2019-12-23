/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceHandler.h>
#include <util/cs_Utils.h>

#include <optional>

#define LOGSwitchAggregator LOGnone

// =====================================================
// ================== Old Owner logic ==================
// =====================================================


bool SwitchAggregator::checkAndSetOwner(cmd_source_t source) {
	if (source.sourceId < CS_CMD_SOURCE_DEVICE_TOKEN) {
		// Non device token command can always set the switch.
		return true;
	}

	if (_ownerTimeoutCountdown == 0) {
		// Switch isn't claimed yet.
		_source = source;
		_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		return true;
	}

	if (_source.sourceId != source.sourceId) {
		// Switch is claimed by other source.
		LOGSwitchAggregator("Already claimed by %u", _source.sourceId);
		return false;
	}

	if (!BLEutil::isNewer(_source.count, source.count)) {
		// A command with newer counter has been received already.
		LOGSwitchAggregator("Old command: %u, already got: %u", source.count, _source.count);
		return false;
	}

	_source = source;
	_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
	return true;
}

SwitchAggregator& SwitchAggregator::getInstance(){
    static SwitchAggregator instance;

    return instance;
}

void SwitchAggregator::init(SwSwitch&& s){
    swSwitch.emplace(s);
    
    this->listen();
    swSwitch->listen();

    twilightHandler.listen();
    behaviourHandler.listen();

    overrideState = swSwitch->getIntendedState();
    swSwitch->resolveIntendedState();
}

void SwitchAggregator::updateState(bool allowOverrideReset){
    if(!swSwitch || !swSwitch->isSwitchingAllowed()){
        // shouldn't update overrideState when switch is locked.
        return;
    }

    // (swSwitch.has_value() is true from here)

    if(overrideState && behaviourHandler.getValue() && aggregatedState){
        bool overrideStateIsOn = *overrideState != 0;
        bool aggregatedStateIsOn = *aggregatedState != 0;
        bool behaviourStateIsOn = *behaviourHandler.getValue() != 0;

        bool overrideMatchedAggregated = overrideStateIsOn == aggregatedStateIsOn;
        bool behaviourWantsToChangeState = behaviourStateIsOn != aggregatedStateIsOn;

        if(overrideMatchedAggregated && behaviourWantsToChangeState && allowOverrideReset){
                // nextAggregatedState = aggregatedBehaviourIntensity();
                LOGSwitchAggregator("resetting overrideState");
                overrideState = {};
        } 
    }

    aggregatedState = 
        overrideState ? resolveOverrideState() : 
        behaviourHandler.getValue() ? aggregatedBehaviourIntensity() : // only use aggr. if no SwitchBehaviour conflict is found
        aggregatedState ? aggregatedState :                            // if conflict is found, don't change the value.
        std::nullopt;

    // DEBUG LOG
    LOGSwitchAggregator("updateState");
    printStatus();
    // END DEBUG LOG
    
    if(aggregatedState){
        swSwitch->setDimmer(*aggregatedState);
    }
}

void SwitchAggregator::printStatus(){
    if(overrideState){
        LOGSwitchAggregator(" ^ overrideState: %02d",overrideState.value());
    }
    if(behaviourHandler.getValue()){
        LOGSwitchAggregator(" | behaviourHandler: %02d",behaviourHandler.getValue().value());
    }
    if(aggregatedState){
        LOGSwitchAggregator(" v aggregatedState: %02d",aggregatedState.value());
    }
}

std::optional<uint8_t> SwitchAggregator::resolveOverrideState(){
    LOGSwitchAggregator("resolveOverrideState called");
    if(*overrideState == 0xff){
        LOGSwitchAggregator("translucent override state");

        if(uint8_t behave_intensity = behaviourHandler.getValue().value_or(0)){
            // behaviour has positive intensity
            LOGSwitchAggregator("behaviour value positive, returning minimum of behaviour(%d) and twilight(%d)", behave_intensity,twilightHandler.getValue());
            return CsMath::min(behave_intensity,twilightHandler.getValue());
        } else {
            // behaviour conflicts or indicates 'off' are ignored because although
            // overrideState is 'translucent', it must be 'on.
            LOGSwitchAggregator("behaviour value undefined or zero, returning twilight(%d)",twilightHandler.getValue());
            return twilightHandler.getValue();
        }
    }
        
    LOGSwitchAggregator("returning unchanged overrideState");
    return overrideState;  // opaque override is unchanged.
}

uint8_t SwitchAggregator::aggregatedBehaviourIntensity(){
    LOGSwitchAggregator("aggregatedBehaviourIntensity called");

    if(behaviourHandler.getValue()){
        LOGSwitchAggregator("returning min of behaviour(%d) and twilight(%d)",
            *behaviourHandler.getValue(),
            twilightHandler.getValue());
        return CsMath::min(
            *behaviourHandler.getValue(), 
            twilightHandler.getValue() 
        );
    }
    LOGSwitchAggregator("returning twilight value(%d), behaviour undefined",twilightHandler.getValue());

    // SwitchBehaviours conflict, so use twilight only
    return twilightHandler.getValue();
}

void SwitchAggregator::handleEvent(event_t& evt){
    if (handleTimingEvents(evt)) {
        return;
    }

    if(handlePresenceEvents(evt)){
        return;
    }

   if(handleAllowedOperations(evt)){
       return;
   }

    handleStateIntentionEvents(evt);    
}

bool SwitchAggregator::handleTimingEvents(event_t& evt){
     switch(evt.type){
        case CS_TYPE::EVT_TICK: {
            // decrement until 0
            _ownerTimeoutCountdown == 0 || _ownerTimeoutCountdown--;
            break;
        }
        case CS_TYPE::STATE_TIME: {
            if(updateBehaviourHandlers()){ 
                updateState();
            }
            
            break;
        }
        case CS_TYPE::EVT_TIME_SET: {
            if(updateBehaviourHandlers()){ 
                updateState();
            }

            break;
        }
        default:{
            return false;
        }
    }

    return true;
}

bool SwitchAggregator::handlePresenceEvents(event_t& evt){
    if(evt.type == CS_TYPE::EVT_PRESENCE_MUTATION){
        PresenceHandler::MutationType mutationtype = *reinterpret_cast<PresenceHandler::MutationType*>(evt.data);

        switch(mutationtype){
            case PresenceHandler::MutationType::LastUserExitSphere : {
                LOGd("SwitchAggregator LastUserExit");
                if(overrideState){
                    TimeOfDay now = SystemTime::now();
                    LOGd("SwitchAggregator LastUserExit override state true (%02d:%02d:%02d)",now.h(),now.m(),now.s());

                    if (behaviourHandler.requiresPresence(now)) {
                        // if there exists a behaviour which is active at given time and 
                        //      	and it has a non-negated presence clause (that may not be satisfied)
                        //      		clear override
                        LOGSwitchAggregator("clearing override state because last user exited sphere");
                        overrideState = {};
                        // updateBehaviourHandlers();
                        // updateState();
                    }
                }
  
                break;
            }
            default: 
                break;
        }

        return true;
    }

    return false;
}

bool SwitchAggregator::updateBehaviourHandlers(){
    bool result = false;
    result |= twilightHandler.update();
    result |= behaviourHandler.update();

    return result;
}

bool SwitchAggregator::handleAllowedOperations(event_t& evt) {
     if (evt.type ==  CS_TYPE::CMD_SWITCH_LOCKED) {
        LOGd("SwitchAggregator::%s case CMD_SWITCH_LOCKED",__func__);
        auto typd = reinterpret_cast<TYPIFY(CMD_SWITCH_LOCKED)*>(evt.data);
        if(swSwitch) swSwitch->setAllowSwitching(*typd);

        evt.result.returnCode = ERR_SUCCESS;

        return true;
    }

    if(swSwitch && !swSwitch->isSwitchingAllowed()){

        evt.result.returnCode = ERR_SUCCESS;

        return true;
    }

    if(evt.type ==  CS_TYPE::CMD_DIMMING_ALLOWED){
        LOGd("SwitchAggregator::%s case CMD_DIMMING_ALLOWED",__func__);
        auto typd = reinterpret_cast<TYPIFY(CMD_DIMMING_ALLOWED)*>(evt.data);
        if(swSwitch) swSwitch->setAllowDimming(*typd);
        
        evt.result.returnCode = ERR_SUCCESS;

        return true;
    }

    return false;
}


bool SwitchAggregator::handleStateIntentionEvents(event_t& evt){
    switch(evt.type){
        // ============== overrideState Events ==============
        case CS_TYPE::CMD_SWITCH_ON:{
            LOGd("CMD_SWITCH_ON",__func__);
            overrideState = 100;
            updateState(false);
            break;
        }
        case CS_TYPE::CMD_SWITCH_OFF:{
            LOGd("CMD_SWITCH_OFF",__func__);
            overrideState = 0;
            updateState(false);
            break;
        }
        case CS_TYPE::CMD_SWITCH: {
            LOGd("CMD_SWITCH",__func__);
			TYPIFY(CMD_SWITCH)* packet = (TYPIFY(CMD_SWITCH)*) evt.data;
            LOGd("packet intensity: %d, source(%d)", packet->switchCmd, packet->source.sourceId);
            if (!checkAndSetOwner(packet->source)) {
                LOGSwitchAggregator("not executing, checkAndSetOwner returned false");
                break;
            }
            
            overrideState = packet->switchCmd;
            updateState(false);
			break;
		}
        case CS_TYPE::CMD_SWITCH_TOGGLE:{
            LOGd("CMD_SWITCH_TOGGLE",__func__);
            // TODO(Arend, 08-10-2019): toggle should be upgraded when twilight is implemented
            overrideState = swSwitch->isOn() ? 0 : 255;
            updateState(false);
            break;
        }

        // ============== 'Developer' Events ==============
        // temporarily change the state of the swSwitch to something else than the
        // behaviourState or overrideState decided that they needed to be.
        // as soon as any of the above handled events are triggered this will have been forgotten.
        case CS_TYPE::CMD_SET_RELAY:{
            LOGd("CMD_SET_RELAY");
            auto typd = reinterpret_cast<TYPIFY(CMD_SET_RELAY)*>(evt.data);
            if(swSwitch) swSwitch->setRelay(*typd);
            break;
        }
        case CS_TYPE::CMD_SET_DIMMER:{
            LOGd("CMD_SET_DIMMER");
            auto typd = reinterpret_cast<TYPIFY(CMD_SET_DIMMER)*>(evt.data);
            if(swSwitch) swSwitch->setIntensity(*typd);
            break;
        }
        default:{
            return false;
        }
    }

    evt.result.returnCode = ERR_SUCCESS;
    return true;
}

void SwitchAggregator::developerForceOff(){
    if(swSwitch){
        swSwitch->setDimmer(0);
        swSwitch->setRelay(false);
    }
}
