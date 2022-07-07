/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 7, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <nrf_dfu_types.h>

/**
 * Handles dfu events and writes status bits in the ipc memory.
 *
 * If P2P_DFU is defined, will also write the init packet of the
 * dfu process to flash.
 */
void dfu_observer(nrf_dfu_evt_type_t evt_type);

void cs_dfu_observer_on_dfu_initialized();
void cs_dfu_observer_on_transport_activated();
void cs_dfu_observer_on_transport_deactivated();
void cs_dfu_observer_on_dfu_started();
void cs_dfu_observer_on_object_received();
void cs_dfu_observer_on_dfu_failed();
void cs_dfu_observer_on_dfu_completed();
void cs_dfu_observer_on_dfu_aborted();
