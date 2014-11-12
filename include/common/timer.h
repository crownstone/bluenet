/**
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#ifndef CS_TIMER_H
#define CS_TIMER_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

	// make callback
	static int timer_flag;

	void timer_config(uint8_t ms);
	void timer_start();
	void timer_disable();

#ifdef __cplusplus
}
#endif

#endif
