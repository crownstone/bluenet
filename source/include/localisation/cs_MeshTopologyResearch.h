/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_MeshTopologyPackets.h>
#include <structs/cs_PacketsInternal.h>
#include <util/cs_Coroutine.h>
#include <util/cs_Variance.h>

/**
 *
 */
class Edge {
public:
	Edge(stone_id_t source, stone_id_t target, int8_t rssi) : source(source), target(target), rssi(rssi) {}

	stone_id_t source;
	stone_id_t target;
	int8_t rssi;
};

class Triangle {
public:
	static constexpr uint8_t NUM_EDGES = 3;

	/**
	 * Edges that make up the triangle.
	 */
	Edge edges[NUM_EDGES];

	/**
	 * Gets the area of the triangle.
	 */
	float getArea();

	/**
	 * Get the angle to the triangle w.r.t. the current crownstone.
	 */
	float getAngle();

	/**
	 * Get the altitude of the triangle.
	 */
	float getAltitude();

	/**
	 *
	 */
	float getBaseLeftSegment();

	/**
	 *
	 */
	float getBaseRightSegment();
};

class MeshTopologyResearch : public EventListener {
public:
	enum TopologyDiscoveryState {
		SCAN_FOR_NEIGHBORS, // Wait for all crownstones that are discoverable my the node to be found 
		BUILD_TRIANGLES, // With the crownstones that are discoverable, start discovering where we can build triangles
		BUILD_TOPOLOGY, // With the discovered triangles begin building a topology
		TOPOLOGY_DONE, // The topology is discovered and valid 
	};

	MeshTopologyResearch();

	/**
	 */
	void handleEvent(event_t& evt);

	/**
	 */
	void init();

private:
};
