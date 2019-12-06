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

#define SWSWITCH_LOG_CALLFLOW LOGnone

#define SWSWITCH_LOG_FUNC() SWSWITCH_LOG_CALLFLOW("SwSwitch::%s",__func__)
#define SWSWITCH_LOCKED_LOG() LOGd("refused because of lock: %s",__func__)


// =========================================================================
// ================================ Private ================================
// =========================================================================

// hardware validation 

bool SwSwitch::startDimmerPowerCheck(uint8_t value){
    if(checkedDimmerPowerUsage || dimmerCheckCountDown != 0 || !isSafeToDim() || value == 0){
        return false;
    }

    SWSWITCH_LOG_FUNC();

    // set the dimmer intensity to [value]
    setIntensity_unchecked(value);
    setRelay_unchecked(false);

    // and set a time out to turn the dimmer back off if it turns out not to work.
    dimmerCheckCountDown = dimmerCheckCountDown_initvalue;

    checkedDimmerPowerUsage = true;
    return true;
}

void SwSwitch::checkDimmerPower() {
    SWSWITCH_LOG_CALLFLOW("checkDimmerPower: allowDimming: %d - currentState.state.dimmer: %d",
        allowDimming, 
        currentState.state.dimmer);

	if (!allowDimming) {
        // doesn't make sense to measure anything when the dimmer is turned off
        // quite a bad condition to be in, better solve it!
        forceDimmerOff();
		return;
	}
    if(currentState.state.dimmer == 0){
        // how did we schedule a dimmer check while value == 0?
        // we can't really make a useful estimate now..
        return;
    }

	// Check if dimmer is on, but power usage is low.
	// In that case, assume the dimmer isn't working due to a cold boot.
	// So turn dimmer off and relay on.

	TYPIFY(STATE_POWER_USAGE) powerUsage;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));
	TYPIFY(CONFIG_POWER_ZERO) powerZero;
	State::getInstance().get(CS_TYPE::CONFIG_POWER_ZERO, &powerZero, sizeof(powerZero));

	SWSWITCH_LOG_CALLFLOW("powerUsage=%i powerZero=%i", powerUsage, powerZero);

	if (powerUsage < DIMMER_BOOT_CHECK_POWER_MW 
		|| (powerUsage < DIMMER_BOOT_CHECK_POWER_MW_UNCALIBRATED 
			&& powerZero == CONFIG_POWER_ZERO_INVALID)
		) {

        if (measuredDimmerPowerUsage){
            // uh oh.. what do we do when dimmer circuit lost power?
            // strange, this must be a second call to checkDimmerPower,
            // which shouldn't occur in the first place.
        }

		measuredDimmerPowerUsage = false;

        forceDimmerOff();
	} else {
        if (!measuredDimmerPowerUsage){
            //Dimmer changed from not yet powered to powered.
            measuredDimmerPowerUsage = true;
        } else {
            // confirmed that the dimmer circuit is still powered. Yay.
        }
    }

    event_t event(CS_TYPE::EVT_DIMMER_POWERED, &measuredDimmerPowerUsage, sizeof(measuredDimmerPowerUsage));
    event.dispatch();
}

bool SwSwitch::isDimmerCircuitPowered(){
    return dimmerPowerUpCountDown == 0 || measuredDimmerPowerUsage;
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

void SwSwitch::store(switch_state_t nextState) {

    bool persistNow = false;

	cs_state_data_t stateData (CS_TYPE::STATE_SWITCH_STATE, 
        reinterpret_cast<uint8_t*>(&nextState), sizeof(nextState));

	if (persistNow) {
		State::getInstance().set(stateData);
	} else {
		State::getInstance().setDelayed(stateData, SWITCH_DELAYED_STORE_MS / 1000);
	}

    // this seems to be redundant as the only call to this function
    // passes currentState as parameter.
    currentState = nextState;

    SWSWITCH_LOG_CALLFLOW("store(%s, %d%%)",
        currentState.state.relay != 0? "on" : "off", 
        currentState.state.dimmer);
}

// forcing hardwareSwitch

void SwSwitch::forceRelayOn() {
    SWSWITCH_LOG_FUNC();

    hwSwitch.setRelay(true);
    actualState.state.relay = 1;
	
	event_t event(CS_TYPE::EVT_RELAY_FORCED_ON);
	EventDispatcher::getInstance().dispatch(event);
	
    // Todo(Arend 21-11-2019) Try again later, in case the first one didn't work..
    // Todo(Arend 01-10-2019) do we really need this? it would be better if 
    // calling scope decides if another forceSwitchOn() call is necessary.
	// delayedSwitch(100, 5);
}

void SwSwitch::forceSwitchOff() {
    SWSWITCH_LOG_FUNC();

	hwSwitch.setIntensity(0);
	hwSwitch.setRelay(false);
    // hwSwitch.setDimmerPower(false);

    switch_state_t nextState;
    nextState.state.dimmer = 0;
    nextState.state.relay = 0;
    store(nextState);

	event_t event(CS_TYPE::EVT_SWITCH_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
	
    // Todo(Arend 21-11-2019) Try again later, in case the first one didn't work..
    // Todo(Arend 01-10-2019) do we really need this? it would be better if 
    // calling scope decides if another forceSwitchOff() call is necessary.
	// delayedSwitch(0, 5);
}

void SwSwitch::forceDimmerOff() {
    SWSWITCH_LOG_FUNC();

    setIntensity_unchecked(0);

    // as there is no mechanism to turn it back on this isn't done yet, 
    // but it would be safer to cut power to the dimmer if in error I suppose.
    // hwSwitch.setDimmerPower(false);

	event_t event(CS_TYPE::EVT_DIMMER_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
}

void SwSwitch::setIntensity_unchecked(uint8_t intensity_value){
    if(actualState.state.dimmer == intensity_value){ 
        return; 
    }

    hwSwitch.setIntensity(intensity_value);
    actualState.state.dimmer = intensity_value;
}

void SwSwitch::setRelay_unchecked(bool relay_state){
    if(actualState.state.relay == relay_state){ 
        return; 
    }

    hwSwitch.setRelay(relay_state);
    actualState.state.relay = relay_state;
}

// ========================================================================
// ================================ Public ================================
// ========================================================================

// ================== SwSwitch ================

SwSwitch::SwSwitch(HwSwitch hw_switch): hwSwitch(hw_switch){
    SWSWITCH_LOG_FUNC();

    // SwSwitch always constructs in 'off' state
    actualState = {0};
    currentState = {0};

    // load intendedState currentState from State.
    State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &currentState, sizeof(currentState));

    // load locked state
    bool switch_locked = false;
    State::getInstance().get(CS_TYPE::CONFIG_SWITCH_LOCKED, &switch_locked, sizeof(switch_locked));
    allowSwitching = ! switch_locked;

    // load dimming allowed stated
    State::getInstance().get(CS_TYPE::CONFIG_PWM_ALLOWED, &allowDimming, sizeof(allowDimming));

    LOGd("SwSwitch constructed in state: relay(%d), intensity(%d), lock(%d), dimming(%d)", 
         currentState.state.relay, 
         currentState.state.dimmer, 
         switch_locked ? 1 : 0, 
         allowDimming ? 1 : 0);

    // ensure the dimmer gets power as the rest of the SwSwitch code assumes 
    // that setIntensity calls will have visible effect.
    // note: dimmer power is not persisted, so we don't need to store this anywhere.
    hwSwitch.setDimmerPower(true);
}

void SwSwitch::setAllowSwitching(bool allowed) {
    SWSWITCH_LOG_CALLFLOW("setAllowSwitching(%s)", allowed ? "true" : "false");
    allowSwitching = allowed;
    bool switch_locked = !allowed;
    State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &switch_locked, sizeof(switch_locked));
}

void SwSwitch::setAllowDimming(bool allowed) {
    if(!allowSwitching){
        SWSWITCH_LOCKED_LOG();
        return;
    }

    SWSWITCH_LOG_CALLFLOW("setAllowDimming(%s)", allowed ? "true" : "false");

    allowDimming = allowed;
    State::getInstance().set(CS_TYPE::CONFIG_PWM_ALLOWED, &allowed, sizeof(allowed));

    // resolve possible inconsistent state.
    if(!allowDimming || hasDimmingFailed()){
        // fix state on disallow
        if(currentState.state.dimmer != 0){
            // turn on to full power if we were dimmed to some
            // percentage, but aren't allowed to.
            setRelay(true);
        } else {
            setRelay(false);
        }

        setIntensity(0);
    }
}

void SwSwitch::resolveIntendedState(){
    SWSWITCH_LOG_CALLFLOW("resolveIntendedstate");

    bool relay_on = currentState.state.relay != 0;
    auto value = currentState.state.dimmer;

    if(relay_on){
        SWSWITCH_LOG_CALLFLOW("resolve: relay on");
        // relay probably is in the correct state already,
        // but that is up to hwSwitch to decide.
        setRelay_unlocked(true);
        setIntensity_unlocked(0);
        return;
    }

    if(value > 0 && (!allowDimming || !isSafeToDim()) ){
        LOGw("resolve: clamp intensity because not %s to dim",
            !allowDimming? "allowed" : "safe");

        if (value > 5){
            LOGw("resolved: on");
            setRelay_unlocked(true);
            setIntensity_unlocked(0);
        } else {
            LOGw("resolved: off");
            setIntensity_unlocked(0);
            setRelay_unlocked(false);
        }

        return;
    }

    if(allowDimming 
            && !checkedDimmerPowerUsage 
            && !isDimmerCircuitPowered()
            && value > 0) {
        // ignore isSafeToDim() value here, we still need to measure that.

        // TODO(Arend): maybe round value up to a minimal test intensity?
        LOGd("resolve: start dimmer power check");
        
        // Ensure that if the dimmer circuit isn't powered yet
        // the dimmer will be switched off eventually.
        // (Which will revert the intensity to 0 if unsuccessful)
        startDimmerPowerCheck(value);

        return;
    }

    // SLERP with actualState?
    SWSWITCH_LOG_CALLFLOW("resolve: default case, using igbts only");

    // set[Intensity,Relay] check error conditions and won't do anything if there is a problem.
    setIntensity_unlocked(value);
    setRelay_unlocked(false);
}

void SwSwitch::setDimmer(uint8_t value){
    if(!allowSwitching){
        SWSWITCH_LOCKED_LOG();
        return;
    }

    SWSWITCH_LOG_CALLFLOW("SwSwitch::setDimmer(%d) called",value);

    currentState.state.dimmer = CsMath::clamp(value,0,100);
    currentState.state.relay = 0;

    store(currentState);

    resolveIntendedState();
}

// ================== ISwitch ==============

void SwSwitch::setRelay(bool is_on){
    if(!allowSwitching){
        SWSWITCH_LOCKED_LOG();
        return;
    }

    setRelay_unlocked(is_on);
}

void SwSwitch::setIntensity(uint8_t value){
    if(!allowSwitching){
        SWSWITCH_LOCKED_LOG()
        return;
    }

    setIntensity_unlocked(value);
}

void SwSwitch::setRelay_unlocked(bool is_on){
    SWSWITCH_LOG_CALLFLOW("SetRelay_unlocked(%d)", is_on);

    if(is_on && !isSafeToTurnRelayOn()){
        LOGw("SetRelay_unlocked refused: unsafe");
        return;
    }
    if(!is_on && !isSafeToTurnRelayOff()){
        LOGw("SetRelay_unlocked refused: unsafe");
        return;
    }

    setRelay_unchecked(is_on);
}

void SwSwitch::setIntensity_unlocked(uint8_t value){;

    SWSWITCH_LOG_CALLFLOW("SetIntensity_unlocked(%d)", value);

    if( value > 0 
        &&( !allowDimming 
            || !isSafeToDim()
            || !isDimmerCircuitPowered()) ){
        LOGw("SetIntensity_unlocked refused: unsafe");
        return;
    }

    setIntensity_unchecked(value); 
}

void SwSwitch::setDimmerPower(bool is_on){
    if(!allowSwitching){
        SWSWITCH_LOCKED_LOG();
        return;
    }

    hwSwitch.setDimmerPower(is_on);
}

// ================== EventListener =============

void SwSwitch::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER:
        case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD:
        case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
            SWSWITCH_LOG_FUNC();
            // First set relay on, so that the switch doesn't first turn off, and later on again.
            // The relay protects the dimmer, because it opens a parallel circuit for the current
            // to flow through.
            forceRelayOn();
            forceDimmerOff();
            break;
        case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
        case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
            SWSWITCH_LOG_FUNC();
            forceSwitchOff();
            break;
        case CS_TYPE::STATE_SWITCH_STATE: {
        	__attribute__((unused)) switch_state_t* typd = reinterpret_cast<TYPIFY(STATE_SWITCH_STATE)*>(evt.data);
            SWSWITCH_LOG_CALLFLOW("switch state update: relay(%d) dim(%d)", typd->state.relay, typd->state.dimmer);
            break;
        }
        case CS_TYPE::EVT_TICK: {
            if (dimmerPowerUpCountDown && --dimmerPowerUpCountDown == 0){
                SWSWITCH_LOG_CALLFLOW("dimmerPowerUpCountDown timed out");

                // update service data through an event as it may not have happened before.
                bool powered = isDimmerCircuitPowered();
                event_t event(CS_TYPE::EVT_DIMMER_POWERED, &powered, sizeof(powered));
                event.dispatch();

                // should only be called if resolving slerps, because otherwise it is too abrupt.
                // resolveIntendedState();
            }
            if(dimmerCheckCountDown && --dimmerCheckCountDown == 0){
                SWSWITCH_LOG_CALLFLOW("dimmerCheckCountDown timed out");
                checkDimmerPower();

                // see if a possible cold dimmer can be fixed 
                resolveIntendedState();
            }
            // TODO: error check in case primary fault-event did not solve the issue?
            // TODO: if currentState != actualState, try to resolveIntendedState() every once in a while?
            break;
        }
        default: {
            // early return prevents setting event return type to successful
            return;
        }
    }

    evt.result.returnCode = ERR_SUCCESS;
}

