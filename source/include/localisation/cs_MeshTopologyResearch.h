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
#include <util/cs_Math.h>

/**
 * @brief Transformations class
 * 
 */
class Transformations {
public:

	/**
	 * @brief Convert RSSI to distance.
	 * 
	 * @param rssi 
	 * @return float 
	 */
	static float rssToDistance(int8_t rssi);

	/**
	 * @brief Roll transformation.
	 * 
	 * @param phi 
	 * @return std::vector<float> 
	 */
	static std::vector<float> roll(float phi);

	/**
	 * @brief Pitch transformation.
	 * 
	 * @param theta 
	 * @return std::vector<float> 
	 */
	static std::vector<float> pitch(float theta); 

	/**
	 * @brief Yaw transformation.
	 * 
	 * @param psi 
	 * @return std::vector<float> 
	 */
    static std::vector<float> yaw(float psi);

	/**
	 * @brief Multiply two matrices or matrix with vector
	 * 
	 * @param matrix 
	 * @param matvec 
	 * @return std::vector<float> 
	 */
	std::vector<float> matrix3_multiply(const std::vector<float>& matrix, const std::vector<float>& vectMatrix);
};

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
	static constexpr uint8_t NUM_EDGES = 3;

	/**
	 * @brief Construct a new Triangle object of three edge pointers
	 * 
	 * @param adj_edge1 
	 * @param adj_edge2 
	 * @param opposite_edge 
	 */
	Triangle(Edge *adj_edge1, Edge *adj_edge2, Edge *opposite_edge) : edges{adj_edge1, adj_edge2, opposite_edge} {}
    // OR
	Triangle(Edge *base_edge, Edge *adj_edge, Edge *opposite_edge) : base_edge(base_edge), adj_edge(adj_edge), opposite_edge(opposite_edge) {}
	
	/**
	 * Edges that make up the triangle.
	 */
	Edge edges[NUM_EDGES];
	// OR
	Edge *base_edge;
	Edge *adj_edge;
	Edge *opposite_edge;

	// TODO: add triangle id, needed?

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

	/**
	 * @brief Returns the third node of the triangle based on the base edge.
	 * Base edge is alway self->otherNode
	 * 
	 * @param baseEdge 
	 * @return stone_id_t 
	 */
	stone_id_t getThirdNode(Edge& baseEdge); 

	/**
	 * @brief Get the Other outgoing base edge
	 * 
	 * @param baseEdge 
	 * @return Edge pointer 
	 */
	Edge* getOtherBase(Edge& baseEdge);

	/**
	 * @brief Get a specific edge from the triangle.
	 * 
	 * @param source 
	 * @param target 
	 * @return Edge pointer
	 */
	Edge* getEdge(stone_id_t source, stone_id_t target);
	
};

/**
 * @brief Tetrahedon class
 * maybe needed
 */

class MeshTopologyResearch : public EventListener {
public:
	enum TopologyDiscoveryState {
		SCAN_FOR_NEIGHBORS, // Wait for all crownstones that are discoverable my the node to be found 
		BUILD_TRIANGLES, // With the crownstones that are discoverable, start discovering where we can build triangles
		BUILD_TOPOLOGY, // With the discovered triangles begin building a topology
		TOPOLOGY_DONE, // The topology is discovered and valid 
	};

	// MeshTopology has list of neighbours, private variable, neighbour got stone id, rssi, lastseen. 
	// Rssi is stored in buffer cs_result_t
	// Reuse this by making it public??

	// array of neighbours (surrounding nodes, sorted on RSSI)
	// Neighbour* neighbourList;

	// array of pointers to edges called edgeList (formed after conformation with other node)
	// Edge* edgeList;

	// array of pointers to triangles called triangleList (formed after conformation with other node)
	// Triangle* triangleList;

	MeshTopologyResearch();
	void handleEvent(event_t& evt);

	void init();
	void triangles();
	void topology();



private:

	// all the procedure functions?

};
