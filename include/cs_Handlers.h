/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <pstorage.h>

#include "drivers/cs_Serial.h"
#include "third/protocol/rbc_mesh.h"

/**Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * This function is called from the scheduler in the main loop after a BLE stack
 *   event has been received.
 *
 * @p_ble_evt   Bluetooth stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt) {

	//LOGi("Sys evt dispatch");

#if CHAR_MESHING==1
	rbc_mesh_sd_irq_handler();
#endif

	if ((sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS) ||
		(sys_evt == NRF_EVT_FLASH_OPERATION_ERROR)) {
		//LOGi("Flash evt dispatch");
		pstorage_sys_event_handler(sys_evt);
	}
}

#ifdef __cplusplus
}
#endif
