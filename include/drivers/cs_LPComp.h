/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

#include "events/cs_Dispatcher.h"
#include "cs_PWM.h"

class LPComp: public Dispatcher {
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

		Event_t getLastEvent() { return _lastEvent; }

		// function to be called from interrupt, do not do much there!
		void update(Event_t event);

		// each program tick, we have time to dispatch events e.g.
		void tick();

	protected:

	private:
		LPComp();
		LPComp(LPComp const&); // singleton, deny implementation
		void operator=(LPComp const &); // singleton, deny implementation
		~LPComp();

		Event_t _lastEvent;
};
