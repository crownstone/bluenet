/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

//#include <stdint.h>
//
//#include <common/cs_Types.h>
//#include "cs_PWM.h"

class LPComp {
	public:
		// use static variant of singleton, no dynamic memory allocation
		static LPComp& getInstance() {
			static LPComp instance;
			return instance;
		}

		enum Event_t {
			LPC_CROSS=0,
			LPC_UP,
			LPC_DOWN,
			LPC_NONE
		};

		uint32_t config(uint8_t pin, uint8_t level, Event_t event);
		void start();
		void stop();

		// function to be called from interrupt, do not do much there!
		void interrupt();

	protected:

	private:
		LPComp();
		LPComp(LPComp const&); // singleton, deny implementation
		void operator=(LPComp const &); // singleton, deny implementation
		~LPComp();

};
