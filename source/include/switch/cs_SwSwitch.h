/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Serial.h>
#include <events/cs_EventListener.h>
#include <switch/cs_HwSwitch.h>
#include <switch/cs_ISwitch.h>

// forward declaration used for friend definition.
class SwitchAggregator;

/**
 * Wrapper object to the underlying HwSwitch that adds safety features and 
 * software configurable settings.
 * 
 * Provides a simple interface to operate the switch.
 */
class SwSwitch : public ISwitch, public EventListener {
    private:
    HwSwitch hwSwitch;
    switch_state_t actualState = {0};

    // sometimes this may differ from the actualState because of
    // hardware errors or startup intricacies.
    // currentState will not change when locked, but may be set to a
    // dimming value other than 0 or 100 even when dimming is not allowed.
    // the latter makes it possible to slow-dim to the intended dimming value
    // when the dimmer takes long to initialize.
    switch_state_t currentState = {0};

    // user/app setting.
    // when resolving currentState this will decide if the dimmer may be used or not.
    bool allowDimming = false;     

    // user/app setting "locked" if false. This prevents all public
    // methods except setAllowSwitching and handleEvent from having any effect.
    bool allowSwitching = true;

    // hardware validation

    // time it takes for a dimmer power check to finish
    const uint32_t dimmerCheckCountDown_initvalue = DIMMER_BOOT_CHECK_DELAY_MS / TICK_INTERVAL_MS;
    uint32_t dimmerCheckCountDown = 0;

    // after this time, assume dimmer is powered  no matter what
    uint32_t dimmerPowerUpCountDown = PWM_BOOT_DELAY_MS / TICK_INTERVAL_MS; 
  
    // flag to prevent multiple power checks, will be set by EVT_TICK handler after calling checkDimmerPower
    bool checkedDimmerPowerUsage = false; 

    // result of checkDimmerPower, true indicates that the dimmer is powered/ready for use.
    bool measuredDimmerPowerUsage = false; 

    /**
     * Initializes the count down timer and sets the dimmer to given
     * value.
     */
    bool startDimmerPowerCheck(uint8_t value);
    /**
     * Returns true if the dimmerCheckCountDown has expired or
     * if a dimmer power check has successfully measured a correct state.
     */
    bool isDimmerCircuitPowered();

    /**
     * try to check if the dimmer is powered and save the value into
     * isDimmerCircuitPowered.
     * 
     * If dimming isn't allowed or currentState has a dimmer intensity of
     * zero, this method will have no effect.
     * 
     * Will fire a EVT_DIMMER_POWERED event when it detects power while 
     * previous know state was unpowered.
     */
    void checkDimmerPower();

    // error state semantics
    TYPIFY(STATE_ERRORS) getErrorState();
    bool hasDimmingFailed(); 
    bool isSwitchFried();
    bool errorStateOK();

    // functional semantics (derrived from error state)
    bool isSafeToTurnRelayOn();
    bool isSafeToTurnRelayOff();
    bool isSafeToDim();

    // persistance features: 
    void store(switch_state_t nextState);

    // exceptional methods (ignores error state but logs and persists)
    void forceRelayOn();
    void forceSwitchOff();
    void forceDimmerOff();
    
    // checks for safety issues and then calls the unchecked variant.
    void setRelay_unlocked(bool is_on);
    void setIntensity_unlocked(uint8_t value);

    // all hwSwitch access is looped through these methods.
    // they check if the would actually be a change in value
    // before executing the action.
    void setIntensity_unchecked(uint8_t dimmer_value);
    void setRelay_unchecked(bool relay_state);

    public:
    /**
     * Restores state from persistent memory and tries to recover to that state.
     */
    SwSwitch(HwSwitch hw_switch);

    // SwSwitch

    /**
     * When the intended state and current state do not match, try to
     * get these two more in sync.
     * 
     * This will ignore the [allowSwitching] bool as intendedState will
     * not be changed when that value is false.
     * It will take [allowDimming] and error states into account.
     * 
     * intendedState is not modified.
     */
    void resolveIntendedState();

    // will setRelay(true) if dimmer was active
    void setAllowDimming(bool allowed); 
    
    // if false is passed, no state changes may occur through
    // the ISwitch interface.
    void setAllowSwitching(bool allowed);

    /**
     * This is the smarter version of setDimmer(value).
     * 
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Will prefer to use dimmer features, but if impossible try
     * to solve the request using the relay.
     * 
     * Will attempt to use the dimmer after cold boot (and check if
     * that worked after a few seconds).
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setDimmer(uint8_t value);

    // ISwitch

    /**
     * Checks if the relay is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     * Does not adjust intensity value.
     */
    void setRelay(bool is_on) override;

    /**
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     * Does not adjust relay value.
     */
    void setIntensity(uint8_t intensity_value) override;

    /**
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setDimmerPower(bool is_on) override;

    // EventListener

    /**
     * Listens to Allow Dimming, and Lock events and error states
     */
    virtual void handleEvent(event_t& evt) override;

    // Getters
    bool isSwitchingAllowed() { return allowSwitching; }
    bool isDimmingAllowed() { return allowSwitching; }
    
    uint8_t getIntensity() { return currentState.state.dimmer; }
    bool isRelayOn() { return currentState.state.relay; }

    uint8_t getCurrentState() { return currentState.asInt; }
    uint8_t getIntendedState() { return currentState.asInt; }
    bool isOn() { return currentState.asInt != 0; } // even dimming to 1% means 'on'
};