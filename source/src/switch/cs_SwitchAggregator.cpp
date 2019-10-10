/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>

#include <optional>

// NOTES:
// bool switchcraftEnabled = State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED);

// =====================================================
// ================== Old Owner logic ==================
// =====================================================

// /**
//  * Which source claimed the switch.
//  *
//  * Until timeout, nothing with a different source can set the switch.
//  * Unless that source overrules the current source.
//  */
// cmd_source_t _source = cmd_source_t(CS_CMD_SOURCE_NONE);
// uint32_t _ownerTimeoutCountdown = 0;

// /**
//  * Tries to set source as owner of the switch.
//  * Returns true on success, false if switch is already owned by a different source, and given source does not overrule it.
//  */
// bool Switch::checkAndSetOwner(cmd_source_t source) {
// 	if (source.sourceId < CS_CMD_SOURCE_DEVICE_TOKEN) {
// 		// Non device token command can always set the switch.
// 		return true;
// 	}

// 	if (_ownerTimeoutCountdown == 0) {
// 		// Switch isn't claimed yet.
// 		_source = source;
// 		_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
// 		return true;
// 	}

// 	if (_source.sourceId != source.sourceId) {
// 		// Switch is claimed by other source.
// 		LOGd("Already claimed by %u", _source.sourceId);
// 		return false;
// 	}

// 	if (!BLEutil::isNewer(_source.count, source.count)) {
// 		// A command with newer counter has been received already.
// 		LOGd("Old command: %u, already got: %u", source.count, _source.count);
// 		return false;
// 	}

// 	_source = source;
// 	_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
// 	return true;
// }

SwitchAggregator& SwitchAggregator::getInstance(){
    static SwitchAggregator instance;

    return instance;
}

void SwitchAggregator::init(SwSwitch&& s){
    swSwitch.emplace(s);
    
    EventDispatcher::getInstance().addListener(&swSwitch.value());
    EventDispatcher::getInstance().addListener(this);
}

void SwitchAggregator::handleEvent(event_t& evt){
    switch(evt.type){
        // ============== 'User/App' Events ==============
        case CS_TYPE::CMD_SWITCH_ON:{
            LOGd("SwitchAggregator::%s case CMD_SWITCH_ON",__func__);
            if(swSwitch) swSwitch->setIntensity(100);
            break;
        }
        case CS_TYPE::CMD_SWITCH_OFF:{
            LOGd("SwitchAggregator::%s case CMD_SWITCH_OFF",__func__);
            if(swSwitch) swSwitch->setIntensity(0);
            break;
        }
        case CS_TYPE::CMD_SWITCH_TOGGLE:{
            LOGd("SwitchAggregator::%s case CMD_SWITCH_TOGGLE",__func__);
            if(swSwitch) swSwitch->toggle();
            break;
        }
        case CS_TYPE::CMD_SWITCH: {
            LOGd("SwitchAggregator::%s case CMD_SWITCH",__func__);
			TYPIFY(CMD_SWITCH)* packet = (TYPIFY(CMD_SWITCH)*) evt.data;
            LOGd("packet intensity: %d", packet->switchCmd);
            if(swSwitch) swSwitch->setIntensity(packet->switchCmd);
			break;
		}
        case CS_TYPE::CMD_DIMMING_ALLOWED: {
            LOGd("SwitchAggregator::%s case CMD_DIMMING_ALLOWED",__func__);
            auto typd = reinterpret_cast<TYPIFY(CMD_DIMMING_ALLOWED)*>(evt.data);
            if(swSwitch) swSwitch->setAllowDimming(*typd);
            break;
        }
        case CS_TYPE::CMD_SWITCH_LOCKED: {
            LOGd("SwitchAggregator::%s case CMD_SWITCH_LOCKED",__func__);
            auto typd = reinterpret_cast<TYPIFY(CMD_SWITCH_LOCKED)*>(evt.data);
            if(swSwitch) swSwitch->setAllowSwitching(*typd);
            break;
        }

        // ============== 'Behaviour' Events ==============
        case CS_TYPE::EVT_BEHAVIOUR_SWITCH_STATE : {
            auto typd = reinterpret_cast<TYPIFY(EVT_BEHAVIOUR_SWITCH_STATE)*>(evt.data);
            LOGd("SwitchAggregator::%s case EVT_BEHAVIOUR_SWITCH_STATE value: %d",__func__, *typd);
            behaviourState = *typd;
            break;
        }
        
        // ============== 'Developer' Events ==============
        case CS_TYPE::CMD_SET_RELAY:{
            LOGd("CMD_SET_RELAY");
            auto typd = reinterpret_cast<TYPIFY(CMD_SET_RELAY)*>(evt.data);
            if(swSwitch) swSwitch->setRelay(*typd);
            break;
        }
        case CS_TYPE::CMD_SET_DIMMER:{
            LOGd("CMD_SET_DIMMER");
            auto typd = reinterpret_cast<TYPIFY(CMD_SET_DIMMER)*>(evt.data);
            if(swSwitch) swSwitch->setDimmer(*typd);
        }
        default:{
            break;
        }
    }
}

void SwitchAggregator::developerForceOff(){
    if(swSwitch){
        swSwitch->setDimmer(0);
        swSwitch->setRelay(false);
    }
}