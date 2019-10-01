/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>

SwitchAggregator& SwitchAggregator::getInstance(){
    static SwitchAggregator instance;

    return instance;
}

void SwitchAggregator::init(SwSwitch s){
    swSwitch = s;
    
    EventDispatcher::getInstance().addListener(this);
}

void SwitchAggregator::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::CMD_SWITCH_ON:{
            if(swSwitch) swSwitch->setIntensity(100);
            break;
        }
        case CS_TYPE::CMD_SWITCH_OFF:{
            if(swSwitch) swSwitch->setIntensity(0);
            break;
        }
        case CS_TYPE::CMD_SWITCH_TOGGLE:{
            if(swSwitch) swSwitch->toggle();
            break;
        }
        case CS_TYPE::CMD_SWITCH: {
			TYPIFY(CMD_SWITCH)* packet = (TYPIFY(CMD_SWITCH)*) evt.data;		
            if(swSwitch) swSwitch->setIntensity(packet->switchCmd);
			break;
		}
        default:{
            break;
        }
    }
}

void SwitchAggregator::developerForceOff(){
    developerSetRelay(false);
    developerSetIntensity(0);
    developerSetDimmer(false);
}