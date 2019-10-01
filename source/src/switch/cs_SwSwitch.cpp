/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SwSwitch.h>
#include <common/cs_Types.h>
#include <storage/cs_State.h>

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>


#define SWSWITCH_DEBUG_NO_PERSISTANCE (true)
#define SWSWITCH_DEBUG_NO_EVENTS (true)

// =========================================================================
// ================================ Private ================================
// =========================================================================

// hardware validation 

void SwSwitch::checkDimmerPower() {
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
    return !hasDimmingFailed() && !isSwitchFried();
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
	bool persistNow = false;

	if (nextState.asInt != currentState.asInt) {
		persistNow = (nextState.state.relay != currentState.state.relay);
	}

	cs_state_data_t stateData (CS_TYPE::STATE_SWITCH_STATE, 
        &nextState.asInt, sizeof(nextState.asInt));

	if (persistNow) {
		State::getInstance().set(stateData);
	} else {
		State::getInstance().setDelayed(stateData, SWITCH_DELAYED_STORE_MS / 1000);
	}

    currentState = nextState;
}


void SwSwitch::storeRelayStateUpdate(bool is_on){
    switch_state_t nextState = currentState;
    nextState.state.relay = is_on;
    store(nextState);
}

void SwSwitch::storeIntensityStateUpdate(uint8_t intensity){
    switch_state_t nextState = currentState;
    nextState.state.dimmer = intensity;
    store(nextState);
}

// forcing hardwareSwitch

void SwSwitch::forceRelayOn() {
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
	hwSwitch.setIntensity(0);

    // as there is no mechanism to turn it back on this isn't done yet, 
    // but it would be safer to cut power to the dimmer if in error I suppose.
    // hwSwitch.setDimmer(false);

    storeIntensityStateUpdate(0);

	event_t event(CS_TYPE::EVT_DIMMER_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
}

// ========================================================================
// ================================ Public ================================
// ========================================================================

// ================== Life Cycle ================

SwSwitch::SwSwitch(HwSwitch hw_switch): hwSwitch(hw_switch){
    State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &currentState, sizeof(currentState));
}

void SwSwitch::setAllowDimming(bool allowed) {
    allowDimming = allowed;

    if(!allowDimming || hasDimmingFailed()){
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
    if(value > 0 && hasDimmingFailed()){
        // can't turn dimming on when it has failed.

        // Todo: should we try turn on fully if the value big and
        // if value is ambiguous resort to a default?
        return;
    }

    if(!isDimmerCircuitPowered){
        checkDimmerPower();

        if(!isDimmerCircuitPowered){
            // dimmer circuit not powered somehow..
            return;
        }
    }

    // hwSwitch.setDimmer(true); // currently unnecessary because false is never used..

    // first ensure the dimmer value is correct
    hwSwitch.setIntensity(value);
    // and then turn off the relay (if it wasn't already off)
    hwSwitch.setRelay(false);

    // persist the new state.
    storeIntensityStateUpdate(value);
}

void SwSwitch::setDimmer(bool is_on){
    hwSwitch.setDimmer(is_on);
}

// ================== Listener =============

void SwSwitch::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER:
        case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD:
        case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
            // First set relay on, so that the switch doesn't first turn off, and later on again.
            // The relay protects the dimmer, because the current will flow through the relay.
            forceRelayOn();
            forceDimmerOff();
            break;
        case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
        case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
            forceSwitchOff();
            break;
        case CS_TYPE::EVT_DIMMING_ALLOWED: {
            setAllowDimming(
                    *reinterpret_cast<TYPIFY(EVT_DIMMING_ALLOWED)*>(evt.data)
                );
            break;
        }
        case CS_TYPE::EVT_DIMMER_POWERED: {
            // new handle. When device boots, at some point this will fire 
            // and we can hook into that to restore previous dimmer values.
        }
        default: break;
    }
}

