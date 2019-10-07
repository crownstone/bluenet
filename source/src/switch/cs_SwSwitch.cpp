/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SwSwitch.h>
#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <storage/cs_State.h>

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>

#define SWSWITCH_LOG() LOGd("SwSwitch::%s",__func__)

#define SWSWITCH_DEBUG_NO_PERSISTANCE (true)
#define SWSWITCH_DEBUG_NO_EVENTS (true)

// =========================================================================
// ================================ Private ================================
// =========================================================================

// hardware validation 

void SwSwitch::checkDimmerPower() {
    SWSWITCH_LOG();
    LOGd("checkDimmerPower: allowDimming: %d - currentState.state.dimmer: %d", allowDimming, currentState.state.dimmer);
	if (!allowDimming || currentState.state.dimmer == 0) {
		return;
	}

	// Check if dimmer is on, but power usage is low.
	// In that case, assume the dimmer isn't working due to a cold boot.
	// So turn dimmer off and relay on.

	TYPIFY(STATE_POWER_USAGE) powerUsage;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));
	TYPIFY(CONFIG_POWER_ZERO) powerZero;
	State::getInstance().get(CS_TYPE::CONFIG_POWER_ZERO, &powerZero, sizeof(powerZero));

	LOGd("powerUsage=%i powerZero=%i", powerUsage, powerZero);

	if (powerUsage < DIMMER_BOOT_CHECK_POWER_MW 
		|| (powerUsage < DIMMER_BOOT_CHECK_POWER_MW_UNCALIBRATED 
			&& powerZero == CONFIG_POWER_ZERO_INVALID)
		) {

        if (isDimmerCircuitPowered){
            // uh oh.. what do we do when dimmer circuit lost power?
        }
		isDimmerCircuitPowered = false;
	} else {
        if (!isDimmerCircuitPowered){
            //Dimmer has finished powering
            TYPIFY(EVT_DIMMER_POWERED) powered = isDimmerCircuitPowered;
		    event_t event(CS_TYPE::EVT_DIMMER_POWERED, &powered, sizeof(powered));
		    EventDispatcher::getInstance().dispatch(event);

            isDimmerCircuitPowered = true;
        } else {
            // confirmed that the dimmer circuit is still powered. Yay.
        }
    }
}

// error state semantics

TYPIFY(STATE_ERRORS) SwSwitch::getErrorState(){
    TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));

    return stateErrors;
}

bool SwSwitch::errorStateOK(){
    return getErrorState().asInt == 0;
}

bool SwSwitch::isSwitchFried(){
    TYPIFY(STATE_ERRORS) stateErrors = getErrorState();

    return stateErrors.errors.chipTemp 
        || stateErrors.errors.overCurrent;
}

bool SwSwitch::hasDimmingFailed(){
    TYPIFY(STATE_ERRORS) stateErrors = getErrorState();

    return stateErrors.errors.overCurrentDimmer 
		|| stateErrors.errors.dimmerTemp 
		|| stateErrors.errors.dimmerOn;
}

// functional semantics (derrived from error state)

bool SwSwitch::isSafeToTurnRelayOn() {
	return hasDimmingFailed() || !isSwitchFried();
}

bool SwSwitch::isSafeToTurnRelayOff(){
    return !hasDimmingFailed();
}

bool SwSwitch::isSafeToDim(){
    // lazy update
    if(!isDimmerCircuitPowered){
        checkDimmerPower();
    }

    return isDimmerCircuitPowered && !hasDimmingFailed() && !isSwitchFried();
}

// storage functionality

/**
 * Store the state immediately when relay value changed, 
 * as the dim value often changes a lot in bursts.
 * Compare it against given oldVal, instead of switch state 
 * stored in State, as that is always changed immediately
 * 
 * (just not the value on flash).
 */
void SwSwitch::store(switch_state_t nextState) {
    SWSWITCH_LOG();
    LOGd("nextState: relay %d dim %d", nextState.state.relay,nextState.state.dimmer);
	bool persistNow = false;
    // TODO(Arend): handle persistNow/later...

	if (nextState.asInt != currentState.asInt) {
		persistNow = (nextState.state.relay != currentState.state.relay);
	}

    LOGd("nextState: %x -> %d", 
         reinterpret_cast<uint8_t*>(&nextState),
        *reinterpret_cast<uint8_t*>(&nextState));

	cs_state_data_t stateData (CS_TYPE::STATE_SWITCH_STATE, 
        reinterpret_cast<uint8_t*>(&nextState), sizeof(nextState));

    LOGd("stateData.value: %x -> %d", 
         reinterpret_cast<uint8_t*>(stateData.value),
        *reinterpret_cast<uint8_t*>(stateData.value));

	if (persistNow) {
		State::getInstance().set(stateData);
	} else {
		State::getInstance().setDelayed(stateData, SWITCH_DELAYED_STORE_MS / 1000);
	}

    LOGd("nextState: {relay=%d, dim=%x}", nextState.state.relay,nextState.state.dimmer);
    currentState = nextState;
}


void SwSwitch::storeRelayStateUpdate(bool is_on){
    SWSWITCH_LOG();
    switch_state_t nextState = currentState;
    nextState.state.relay = is_on;
    store(nextState);
}

void SwSwitch::storeIntensityStateUpdate(uint8_t intensity){
    SWSWITCH_LOG();
    switch_state_t nextState = currentState;
    nextState.state.dimmer = intensity;
    store(nextState);
}

// forcing hardwareSwitch

void SwSwitch::forceRelayOn() {
    SWSWITCH_LOG();
    hwSwitch.setRelay(true);
    storeRelayStateUpdate(true);
	
	event_t event(CS_TYPE::EVT_RELAY_FORCED_ON);
	EventDispatcher::getInstance().dispatch(event);
	
    // Try again later, in case the first one didn't work..
    // Todo(Arend 01-10-2019) do we really need this? it would be better if 
    // calling scope decides if another forceSwitchOn() call is necessary.
	// delayedSwitch(100, 5);
}

void SwSwitch::forceSwitchOff() {
    SWSWITCH_LOG();
	hwSwitch.setIntensity(0);
	hwSwitch.setRelay(false);
    // hwSwitch.setDimmer(false);

    switch_state_t nextState;
    nextState.state.dimmer = 0;
    nextState.state.relay = 0;
    store(nextState);

	event_t event(CS_TYPE::EVT_SWITCH_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
	
    // Try again later, in case the first one didn't work..
    // Todo(Arend 01-10-2019) do we really need this? it would be better if 
    // calling scope decides if another forceSwitchOff() call is necessary.
	// delayedSwitch(0, 5);
}

void SwSwitch::forceDimmerOff() {
    SWSWITCH_LOG();
	hwSwitch.setIntensity(0);

    // as there is no mechanism to turn it back on this isn't done yet, 
    // but it would be safer to cut power to the dimmer if in error I suppose.
    // hwSwitch.setDimmer(false);

    storeIntensityStateUpdate(0);

	event_t event(CS_TYPE::EVT_DIMMER_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
}

void SwSwitch::resetToCurrentState(){
    SWSWITCH_LOG();
    switch_state_t actualNextState = {0};

    if(currentState.state.dimmer){
        if(isDimmerCircuitPowered && isSafeToDim()){
            hwSwitch.setIntensity(currentState.state.dimmer);
            hwSwitch.setRelay(false);

            actualNextState.state.dimmer = currentState.state.dimmer;
            actualNextState.state.relay = currentState.state.relay;
        }
    } else {
        if(currentState.state.relay){
            if(isSafeToTurnRelayOn()){
                hwSwitch.setRelay(true);
                hwSwitch.setIntensity(0);
                
                actualNextState.state.relay = true;
                actualNextState.state.dimmer = 0;
            }
        } else {
            if(isSafeToTurnRelayOff()){
                hwSwitch.setRelay(false);
                hwSwitch.setIntensity(0);

                actualNextState.state.relay = false;
                actualNextState.state.dimmer = 0;
            }
        }
    }

    store(actualNextState);
}

// ========================================================================
// ================================ Public ================================
// ========================================================================

// ================== SwSwitch ================

SwSwitch::SwSwitch(HwSwitch hw_switch): hwSwitch(hw_switch){
    SWSWITCH_LOG();
    // load currentState from State. 
    State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &currentState, sizeof(currentState));

    // ensure the dimmer gets power as the rest of the SwSwitch code assumes 
    // that setIntensity calls will have visible effect.
    // note: dimmer power is not persisted, so we don't need to store this anywhere.
    hwSwitch.setDimmer(true);

    resetToCurrentState();
}

void SwSwitch::setAllowDimming(bool allowed) {
    SWSWITCH_LOG();
    allowDimming = allowed;

    if (allowed && State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
		LOGw("unlock switch because dimmer turned on");
		TYPIFY(CONFIG_SWITCH_LOCKED) lockEnable = false;
		State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &lockEnable, sizeof(lockEnable));
	}

    if(!allowDimming || hasDimmingFailed()){
        // fix state on disallow
        setIntensity(0);

        if(currentState.state.dimmer != 0){
            // turn on to full power if we were dimmed to some
            // percentage, but aren't allowed to.
            setRelay(true);
        }
    }
}

// ================== ISwitch ==============

void SwSwitch::setRelay(bool is_on){
    SWSWITCH_LOG();
    if(is_on && !isSafeToTurnRelayOn()){
        // intent to turn on, but not safe
        return;
    }
    if(!is_on && !isSafeToTurnRelayOff()){
        // intent to turn off, but not safe
        return;
    }

    hwSwitch.setRelay(is_on);
    storeRelayStateUpdate(is_on);
}

void SwSwitch::setIntensity(uint8_t value){
    SWSWITCH_LOG();
    LOGd("intensity: %d",value);

    if(value > 0 && ! isSafeToDim()){
        // can't turn dimming on when it has failed.

        if (value > 75){
            // Todo(Arend): double check if this is desired.
            setRelay(true);
            setIntensity(0);
        }

        // Todo(Arend): if value is ambiguous (~50) resort to a default?

        return;
    }

    // first ensure the dimmer value is correct
    hwSwitch.setIntensity(value);
    storeIntensityStateUpdate(value);

    // and then turn off the relay (if it wasn't already off)
    hwSwitch.setRelay(false);
    storeRelayStateUpdate(false);
}

void SwSwitch::setDimmer(bool is_on){
    hwSwitch.setDimmer(is_on);
}

// ================== EventListener =============

void SwSwitch::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER:
        case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD:
        case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
            SWSWITCH_LOG();
            // First set relay on, so that the switch doesn't first turn off, and later on again.
            // The relay protects the dimmer, because the current will flow through the relay.
            forceRelayOn();
            forceDimmerOff();
            break;
        case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
        case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
            SWSWITCH_LOG();
            forceSwitchOff();
            break;
        case CS_TYPE::CONFIG_PWM_ALLOWED: {
            SWSWITCH_LOG();
            setAllowDimming(
                    *reinterpret_cast<TYPIFY(CONFIG_PWM_ALLOWED)*>(evt.data)
                );
            break;
        }
        default: break;
    }
}

