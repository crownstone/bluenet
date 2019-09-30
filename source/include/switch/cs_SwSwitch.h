/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <switch/cs_ISwitch.h>
#include <switch/cs_HwSwitch.h>

#include <events/cs_EventListener.h>

/**
 * Wrapper object to the underlying HwSwitch that adds safety features and 
 * software configurable settings.
 * 
 * Provides a simple interface to operate the switch.
 */
class SwSwitch : public ISwitch, public EventListener {
    private:
    HwSwitch hwSwitch;
    switch_state_t currentState = {0};

    bool allowDimming = false;     // user/app setting
    bool allowSwitching = true;    // user/app setting "locked" if false

    // error state semantics
    TYPIFY(STATE_ERRORS) getErrorState();
    bool hasDimmingFailed(); 
    bool isSwitchFried();
    bool errorStateOK();

    // functional semantics (derrived from error state)
    bool isSafeToTurnRelayOn();
    bool isSafeToTurnRelayOff();
    bool isSafeToDim();

    // persistance features
    void store(switch_state_t nextState);
    void storeRelayStateUpdate(bool is_on);
    void storeIntensityStateUpdate(uint8_t intensity);
    // void storeDimmerStateUpdate(bool is_on); // not implemented

    // exceptional methods (ignores error state but logs and persists)
    void forceSwitchOff();
    void forceDimmerOff();
    void forceRelayOn();

    public:
    SwSwitch(HwSwitch hw_switch): hwSwitch(hw_switch){}

    void setAllowDimming(bool allowed); // will setRelay(true) if dimmer was active
    void setAllowSwitching(bool allowed) {allowSwitching = allowed;}


    // ISwitch

    /**
     * Checks if the relay is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setRelay(bool is_on);

    /**
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setDimmer(bool is_on);

    /**
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setIntensity(uint8_t value);

    // EventListener

    /**
     * Listens to Allow Dimming, and Lock events and error states
     */
    virtual void handleEvent(event_t& evt) override;

    // Other functions

    switch_state_t getSwitchState() {return currentState; }

    // todo: remove this function or add an actual implementation
    template<class T, class S>
    void delayedSwitch(T,S){}
};