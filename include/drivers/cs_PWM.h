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

#define ERR_PWM_NOT_ENABLED 1

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
	uint32_t init(app_pwm_config_t & config, bool inverted);

	//! Returns configuration values for 1 Channel
	app_pwm_config_t & config1Ch(uint32_t period_us, uint32_t pin, bool inverted);

	//! Returns configuration values for 2 Channels
	app_pwm_config_t & config2Ch(uint32_t period_us, uint32_t pin1, uint32_t pin2, bool inverted);

	//! De-Initialize the PWM instance, i.e. free allocated resources
	uint32_t deinit();

	//! Set the value of a specific channel
	void setValue(uint8_t channel, uint32_t value);

	//! Switch off all channels
	//! Also works when not initialized (useful for emergencies)
	void switchOff();

	//! Get current value from unit
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
#if (NORDIC_SDK_VERSION >= 11) //! Not sure if 11 is the first version
	app_pwm_cb_t _controlBlock;
#else
	uint32_t _controlBlock[APP_PWM_CB_SIZE];
#endif

	//! Timer handling PWM
	nrf_drv_timer_t* pwmTimer;
	//! PWM instance
	app_pwm_t* _pwmInstance;

	//! Flag to indicate that the init function has been successfully performed
	bool _initialized;

	//! Invert switch mode
	bool _inverted;
};
