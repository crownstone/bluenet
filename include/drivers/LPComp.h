/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#ifndef __LPCOMP__H__
#define __LPCOMP__H__

#include <stdint.h>
#include <events/Dispatcher.h>
#include <drivers/nrf_pwm.h>

class LPComp: public Dispatcher {
	public:
		// use static variant of singelton, no dynamic memory allocation
		static LPComp& getInstance() {
			static LPComp instance;
			return instance;
		}

		enum Event_t {
			CROSS=0,
			UP,
			DOWN,
			NONE
		};

		uint32_t config(uint8_t pin, uint8_t level, Event_t event);
		void start();
		void stop();

		Event_t getLastEvent() { return lastEvent; }

		void setPWM();


		// function to be called from interrupt, do not do much there!
		void update(Event_t event);

		// in the program loop, time to dispatch events e.g.
		void tick();

	protected:

	private:
		LPComp();
		LPComp(LPComp const&); // singleton, deny implementation
		void operator=(LPComp const &); // singleton, deny implementation
		~LPComp();

		Event_t lastEvent;
};

#endif
