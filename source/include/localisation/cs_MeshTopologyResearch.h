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
 * @brief Edge class
 * 
 */
class Edge {
public:
	Edge(stone_id_t source, stone_id_t target, int8_t rssi) : source(source), target(target), rssi(rssi) {}

	stone_id_t source;
	stone_id_t target;
	int8_t rssi;
	float distance; // TODO: add conversion distance from rssi

	/**
	 * @brief Compare two edges.
	 * 
	 * @param other 
	 * @return true 
	 * @return false 
	 */
	bool compare(const Edge &other) const;
};

/**
 * @brief Triangle class
 * 
 */
class Triangle {
public:
	/**
	 * @brief Construct a new Triangle object of three edge pointers
	 * 
	 * @param adj_edge1 
	 * @param adj_edge2 
	 * @param opposite_edge 
	 */
	Triangle(Edge *base_edge, Edge *adj_edge, Edge *opposite_edge) : base_edge(base_edge), adj_edge(adj_edge), opposite_edge(opposite_edge) {}
	
	/**
	 * Edges that make up the triangle.
	 */
	Edge *base_edge;
	Edge *adj_edge;
	Edge *opposite_edge;

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
	 * @brief Get the Alitude Base Position of the Altitude's x position to the target crownstone.
	 * 
	 * @param target 
	 * @return float 
	 */
	float getAlitudeBasePositionTo(stone_id_t targetID);

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
	void handleEvent(event_t& evt) {
		routine.handleEvent(evt);
	}

	/**
	 */
	void init();

	Coroutine routine;

private:
};
