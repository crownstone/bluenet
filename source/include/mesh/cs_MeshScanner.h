/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

extern "C" {
#include <nrf_mesh.h>
}

/**
 * Class that handles scans from the mesh.
 *
 * Copies the data into a bluenet struct and dispatches an event.
 */
class MeshScanner {
public:
	/**
	 * Don't dispatch scans when the CPU is busy.
	 * This is measured by how full the scheduler queue is.
	 * Only dispatch when there is at least N free space in the queue.
	 */
	const static uint16_t MIN_SCHEDULER_FREE = SCHED_QUEUE_SIZE / 2;

	/**
	 * Handle a scan by the mesh, and dispatch scanned device event.
	 */
	void onScan(const nrf_mesh_adv_packet_rx_data_t *scanData);
};
