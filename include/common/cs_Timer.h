/**
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <cs_Nordic.h>

class Timer {
private:
	Timer();
	Timer(Timer const&); 
	void operator=(Timer const &); 
public:
	static Timer& getInstance() {
		static Timer instance;
		return instance;
	}

	/* Create timer 
	 * @timer_handle            An id or handle to reference the timer (actually, just a Uint32_t) 
	 * @func                    The function to be called
	 *
	 * Create a timer for a specific purpose.
	 */
	void create(app_timer_id_t & timer_handle, app_timer_timeout_handler_t func);

	/* Start a previously created timer
	 * @timer_handle            Reference to previously created timer
	 * @ticks                   Number of ticks till timeout (minimum is 5)
	 */
	void start(app_timer_id_t & timer_handle, uint32_t ticks);

	/* Stop a timer
	 * @timer_handle            Reference to previously created timer
	 */
	void stop(app_timer_id_t & timer_handle);
};

