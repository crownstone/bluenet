/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */

#ifndef _CS_HANDLERS
#define _CS_HANDLERS

#ifdef __cplusplus
extern "C" {
#endif

#include <common/storage.h>
#include <softdevice_handler.h>

#include <drivers/serial.h>

/**
 * Call all event handlers on a system event.
 */
	static void sys_evt_dispatch(uint32_t sys_evt) {
		//LOGi("Sys evt dispatch");
		if ((sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS) ||
				(sys_evt == NRF_EVT_FLASH_OPERATION_ERROR)) {
			//LOGi("Flash evt dispatch");
			pstorage_sys_event_handler(sys_evt);
		}
	}

#ifdef __cplusplus
}
#endif

#endif // _CS_HANDLERS
