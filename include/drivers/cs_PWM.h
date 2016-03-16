/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <cstdint>

#include <ble/cs_Nordic.h>

extern "C" {
#include "nrf_drv_timer.h"
#include "app_pwm.h"
}

//! To change the timer used for the PWM library replace the defines below
#define PWM_TIMER               NRF_TIMER2
//#define PWM_IRQHandler          TIMER2_IRQHandler
#define PWM_IRQn                TIMER2_IRQn
#define PWM_INSTANCE_INDEX      TIMER2_INSTANCE_INDEX

/** Pulse Wide Modulation class
 *
 * To turn on/off the power, as well as all intermediate stages, for example with dimming, the PWM class is used.
 */
class PWM {
public:
	//! Gets a static singleton (no dynamic memory allocation) of the PWM clss
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}

//	static volatile bool _pwmReady;
//	static void pwmReadyCallback(uint32_t pwmId);

	//! Initialize the pulse wide modulation settings
	uint32_t init(app_pwm_config_t config);

	//! Returns configuration values for 1 Channel
	static app_pwm_config_t config1Ch(uint32_t period_us, uint32_t pin);
	//! Returns configuration values for 2 Channels
	static app_pwm_config_t config2Ch(uint32_t period_us, uint32_t pin1, uint32_t pin2);

	//! De-Initialize the PWM instance, i.e. free allocated resources
	uint32_t deinit();

	//! Set the value of a specific channel
	void setValue(uint8_t channel, uint32_t value);

	//! Switch off all channels
	//! Also works when not initialized (useful for emergencies)
	void switchOff();

	uint32_t getValue(uint8_t channel);
	
private:
	//! Private PWM constructor
	PWM();

	//! Private PWM copy constructor
	PWM(PWM const&);
	//! Private PWM copy assignment definition
	void operator=(PWM const &);

	//! PWM configuration
	app_pwm_config_t _pwmCfg;
	//! Array holding ready callbacks for the PWM instance
	uint32_t _callbackArray[APP_PWM_CB_SIZE];

	//! Timer handling PWM
	nrf_drv_timer_t* pwmTimer;
	//! PWM instance
	app_pwm_t* _pwmInstance;

	bool _initialized;
};
