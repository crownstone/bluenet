/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 24, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdint>

#include <ble/cs_Nordic.h>

#ifdef __cplusplus
extern "C" {
#endif

void sys_evt_dispatch(uint32_t sys_evt);

//void ble_evt_handler(ble_evt_t* p_ble_evt);

#ifdef __cplusplus
}
#endif


