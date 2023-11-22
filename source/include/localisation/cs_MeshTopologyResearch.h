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

// typedef uint8_t edge_id_t;
typedef uint8_t triangle_id_t;

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
	static float rssiToDistance(int8_t rssi);

	/**
	 * @brief Roll transformation.
	 * 
	 * @param phi 
	 * @return std::vector<float> 
	 */
	static float* roll(float phi);

	/**
	 * @brief Pitch transformation.
	 * 
	 * @param theta 
	 * @return std::vector<float> 
	 */
	static float* pitch(float theta); 

	/**
	 * @brief Yaw transformation.
	 * 
	 * @param psi 
	 * @return std::vector<float> 
	 */
    static float* yaw(float psi);

	/**
	 * @brief Mirror X-value transformation.
	 * @return float* 
	 */
	static float* mirrorX();

	/**
	 * @brief Mirror Y-value transformation.
	 * @return float* 
	 */
	static float* mirrorY();

	/**
	 * @brief Mirror Z-value transformation.
	 * @return float* 
	 */
	static float* mirrorZ();

	/**
	 * @brief Multiply two matrices or matrix with vector
	 * 
	 * @param matrix 
	 * @param matvec 
	 * @return std::vector<float> 
	 */
	float* matrix3_multiply(const float* matrix, const float* vectMatrix, size_t size);

};

/**
 * @brief Edge class
 * 
 */
class Edge {
public:

	Edge(stone_id_t source, stone_id_t target, int8_t rssi);

	stone_id_t source;
	stone_id_t target;
	int8_t rssi;
	float distance;

	/**
	 * @brief Compare two edges.
	 * 
	 * @param other 
	 * @return true 
	 * @return false 
	 */
	bool compare(const Edge *other) const;

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

	// TODO: add triangle id, needed?
	triangle_id_t id = 0;

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

class MeshTopologyResearch: public EventListener {
public:

	enum TopologyDiscoveryState {
		SCAN_FOR_NEIGHBORS, // Wait for all crownstones that are discoverable my the node to be found 
		BUILD_TRIANGLES, // With the crownstones that are discoverable, start discovering where we can build triangles
		BUILD_TOPOLOGY, // With the discovered triangles begin building a topology
		TOPOLOGY_DONE, // The topology is discovered and valid 
	};

	struct __attribute__((__packed__)) sur_node_t {
		stone_id_t id;
		int8_t rollingAverageRssi;
		uint8_t count;
		uint8_t lastSeenSecondsAgo;
	};

	MeshTopologyResearch();

	virtual ~MeshTopologyResearch() noexcept {};
	
	/**
	 * Commands handled:
	 *  - CMD_MESH_TOPO_GET_RSSI
	 *  - CMD_MESH_TOPO_RESET
	 *  - CMD_MESH_TOPO_GET_MAC
	 *
	 * Internal usage:
	 *  - EVT_RECV_MESH_MSG
	 */
	void handleEvent(event_t& evt);

	int stateFunc(); // TOOD: give a reasonable name

	cs_ret_code_t init();
	void triangles();
	void topology();

	Coroutine routine;
	TopologyDiscoveryState state = TopologyDiscoveryState::SCAN_FOR_NEIGHBORS;

	/**
	 * @brief Find a surrounding node in the surNodeList.
	 * SurNodeList contains more neighbours than the edgeList.
	 * @param id 
	 * @return uint8_t 
	 */
	uint8_t findSurNode(stone_id_t id);

	/**
	 * @brief Find an edge in the edgeList.
	 * EdgeLists consist solely of the SurNodes associated with formed edges
	 * @param id 
	 * @return uint8_t 
	 */
	uint8_t findEdge(stone_id_t id);

	/**
	 * @brief Find a triangle in the trianglesList.
	 * 
	 * @param id 
	 * @return uint8_t 
	 */
	uint8_t findTriangle(triangle_id_t id);

		// array of neighbours (surrounding nodes, sorted on RSSI)
	static constexpr int MAX_SURROUNDING = 10;
	std::optional<sur_node_t> surNodeList[MAX_SURROUNDING];

	// array of edges (automatic sorted on RSSI due to sorted neighbour list)
	static constexpr int MAX_EDGES = 3;
	std::optional<Edge> edgeList[MAX_EDGES];

	// array of triangles (sorted on area/or other metric)
	static constexpr int MAX_TRIANGLES = 5;
	std::optional<Triangle> trianglesList[MAX_TRIANGLES];

private:

	uint8_t _surNodeCount                    = 0;
	uint8_t _edgeCount                       = 0;
	uint8_t _triangleCount                   = 0;

	static constexpr uint8_t INDEX_NOT_FOUND = 0xFF;

	static constexpr int8_t RSSI_INIT        = 0;  // Should be in protocol

	/**
	 * Resets the stored topology.
	 */
	void reset();

	/**
	 * @brief processes all the neighbour RSSI messages of the mesh
	 * 
	 * @param id 
	 * @param packet 
	 */
	void onNeighbourRssi(stone_id_t id, cs_mesh_model_msg_neighbour_rssi_t& packet);

	/**
	 * @brief Get the Mean of the RSSI packet
	 * 
	 * @param packet 
	 * @return uint8_t 
	 */
	static uint8_t getMean(cs_mesh_model_msg_neighbour_rssi_t& packet);

	/**
	 * @brief Send RSSI to uart
	 * 
	 * @param receiverId 
	 * @param packet 
	 */
	void sendRssiToUart(stone_id_t receiverId, cs_mesh_model_msg_neighbour_rssi_t& packet);

	/**
	 * Handles mesh messages:
	 *  - CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI
	 *  - CS_MESH_MODEL_TYPE_NODE_REQUEST
	 *  - CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST
	 */
	void onMeshMsg(MeshMsgEvent& packet, cs_result_t& result);
	
	/**
	 * @brief Add a surrounding node to the surNodeList
	 * 
	 * @param id 
	 * @param rssi 
	 * @param channel 
	 */
	void addSurNode(stone_id_t id, int8_t rssi);

	/**
	 * @brief Update a surrounding node in the surNodeList or add a new one to the list 
	 * (helperfunction of addSurNode)
	 * 
	 * @param node 
	 * @param id 
	 * @param rssi 
	 */
	void updateSurNode(sur_node_t& node, stone_id_t id, int8_t rssi);

	/**
	 * @brief Copy RSSI of stoneID into a result buffer
	 * 
	 * @param stoneId 
	 * @param result 
	 */
	void getRssi(stone_id_t stoneId, cs_result_t& result);

	/**
	 * @brief Add an edge to the edgelist
	 * 
	 * @param source 
	 * @param target 
	 * @param rssi 
	 */
	void addEdge(stone_id_t source, stone_id_t target, int8_t rssi);

};
