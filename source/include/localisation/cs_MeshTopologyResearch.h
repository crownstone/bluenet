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
#include <util/cs_Math.h>
#include <util/cs_Variance.h>

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
	Edge() = default;

	/**
	 * @brief Construct a new Edge object of two stone IDs and an RSSI value.
	 *
	 * @param source
	 * @param target
	 * @param rssi
	 */
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
	bool compare(const Edge* other) const;
};

/**
 * @brief Triangle class
 *
 */
class Triangle {
public:
	Triangle() = default;

	/**
	 * @brief Construct a new Triangle object of three edge pointers
	 *
	 * @param adj_edge1
	 * @param adj_edge2
	 * @param opposite_edge
	 */
	Triangle(Edge* base_edge, Edge* adj_edge, Edge* opposite_edge)
			: base_edge(base_edge), adj_edge(adj_edge), opposite_edge(opposite_edge) {}

	/**
	 * Edges that make up the triangle.
	 */
	Edge* base_edge;
	Edge* adj_edge;
	Edge* opposite_edge;

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

class MeshTopologyResearch : public EventListener {
public:
	enum TopologyDiscoveryState {
		SCAN_FOR_NEIGHBORS,  // Wait for all crownstones that are discoverable my the node to be found
		BUILD_EDGES,      // With the crownstones that are discoverable, start discovering where we can build edges
		BUILD_TRIANGLES,  // If enough edges, we can build triangles
		BUILD_TOPOLOGY,   // With the discovered triangles begin building a topology
		TOPOLOGY_DONE,    // The topology is discovered and valid
		LOCALISATION,    // The topology is used for localisation
	};

	struct __attribute__((__packed__)) sur_node_t {
		stone_id_t id;
		int8_t rssiChannel37;
		int8_t rssiChannel38;
		int8_t rssiChannel39;
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

	int stateFunc();  // TOOD: give a reasonable name

	/**
	 * Initializes the class:
	 * - Get stone ID.
	 * - Allocate and Reset all buffers.
	 * - Starts listening for events.
	 */
	cs_ret_code_t init();

	Coroutine routine;

	TopologyDiscoveryState state             = TopologyDiscoveryState::SCAN_FOR_NEIGHBORS;

	bool SCAN_FOR_NEIGHBORS_FINISHED = false;
	static constexpr uint8_t TIMEOUT_SEND_NEIGHBOURS   =  10; // 5 * 60;

	bool test = false;

	/**
	 * Time after last seen, before a neighbour is removed from the list.
	 */
	static constexpr uint8_t TIMEOUT_SECONDS = 3 * 60;

	// array of neighbours (surrounding nodes, sorted on RSSI)
	static constexpr int MAX_SURROUNDING     = 5;
	sur_node_t* surNodeList                  = nullptr;

	// array of edges (automatic sorted on RSSI due to sorted neighbour list)
	static constexpr int MAX_EDGES           = 4;
	Edge* edgeList                           = nullptr;

	// array of triangles (sorted on area/or other metric)
	static constexpr int MAX_TRIANGLES       = 2;
	Edge* oppositeEdgeList                   = nullptr;
	Triangle* trianglesList                  = nullptr;

private:
	/**
	 * Stone ID of this crownstone, read on init.
	 */
	stone_id_t _myId                         = 0;

	/**
	 * Number of RSSI messages send via the mesh.
	 */
	uint8_t _msgCount                        = 0;

	/**
	 * Next index of the surrounding node list to send via the mesh.
	 */
	uint8_t _nextPrint                       = 0;

	uint8_t _sendNeighbours 				 = 0;


	/**
	 * Index of surrounding nodes, edges, opposite edges and triangles -List.
	 */
	uint8_t _surNodeCount                    = 0;
	uint8_t _edgeCount                       = 0;
	uint8_t _oppositeEdgeCount               = 0;
	uint8_t _triangleCount                   = 0;

	static constexpr uint8_t INDEX_NOT_FOUND = 0xFF;

	static constexpr int8_t RSSI_INIT        = 0;  // Should be in protocol

	/**
	 * Resets the stored topology.
	 */
	void reset();

	/**
	 * @brief Find a surrounding node in the surNodeList.
	 * SurNodeList contains more neighbours than the edgeList.
	 * @param id
	 * @return uint8_t (index)
	 */
	uint8_t findSurNode(stone_id_t id);

	/**
	 * @brief Find an edge in the edgeList.
	 * EdgeList consist solely of the SurNodes associated with formed edges
	 * @param id
	 * @return uint8_t (index)
	 */
	uint8_t findEdge(stone_id_t id);

	/**
	 * @brief Find an edge in the oppositeEdgeList.
	 * @param id
	 * @return uint8_t (index)
	 */
	uint8_t findOppositeEdge(stone_id_t id);

	/**
	 * @brief Find a triangle in the triangleList.
	 * 
	 * @param base_src 
	 * @param base_dst 
	 * @return uint8_t (index)
	 */
	uint8_t findTriangle(stone_id_t base_src, stone_id_t base_dst);

	/**
	 * @brief Print all surrounding nodes
	 */
	void printSurNodes();

	/**
	 * @brief Print all edges
	 */
	void printEdges();

	/**
	 * @brief Print all triangles
	 */
	void printTriangles();

	/**
	 * @brief processes all the neighbour RSSI messages of the mesh
	 *
	 * @param id
	 * @param packet
	 */
	void onNeighbourRssi(stone_id_t OGsenderID, cs_mesh_model_msg_neighbour_rssi_t& packet);

	/**
	 * @brief Construct a new Neighbour-RSSI-message via Mesh
	 *
	 * @param surNode
	 */
	void sendNeighbourRSSIviaMesh(sur_node_t& surNode);

	/**
	 * @brief Get the Mean of the RSSI packet
	 *
	 * @param packet
	 * @return uint8_t
	 */
	static uint8_t getMean(cs_mesh_model_msg_neighbour_rssi_t& packet);

	/**
	 * Handles mesh messages:
	 *  - CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI
	 *  - CS_MESH_MODEL_TYPE_NODE_REQUEST
	 *  - CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST
	 */
	void onMeshMsg(MeshMsgEvent& packet, cs_result_t& result);

	/**
	 * @brief Tick Timer scheduler: remove surNodes when they are not seen for a while
	 *
	 */
	void onTickSecond();

	/**
	 * clears the RSSI stats values to RSSI_INIT and zero
	 * @param node
	 */
	void clearSurNodeRSSIinfo(sur_node_t& node);

	/**
	 * @brief Add a surrounding node to the surNodeList
	 *
	 * @param id
	 * @param rssi
	 * @param channel
	 */
	void addSurNode(stone_id_t id, int8_t rssi, uint8_t channel);

	/**
	 * @brief Update a surrounding node in the surNodeList or add a new one to the list
	 * (helperfunction of addSurNode)
	 *
	 * @param node
	 * @param id
	 * @param rssi
	 */
	void updateSurNode(sur_node_t& node, stone_id_t id, int8_t rssi, uint8_t channel);

	/**
	 * @brief Sort the surNodeList on RSSI
	 */
	void sortSurNodeList();

	/**
	 * @brief Copy RSSI of stoneID into a result buffer
	 *
	 * @param stoneId
	 * @param result
	 */
	void getRssi(stone_id_t stoneId, cs_result_t& result);

	/**
	 * @brief send Neighbour RSSI message to Uart
	 */
	void sendRssiToUart(stone_id_t receiverId, cs_mesh_model_msg_neighbour_rssi_t& packet);

	/**
	 * @brief Add an edge to the edgelist
	 *
	 * -@param source (always self)
	 * @param target
	 * @param rssi
	 *
	 * @return Edge pointer
	 */
	Edge* addEdge(stone_id_t target, int8_t rssi);

	/**
	 * @brief Add an edge to the oppositeEdgelist
	 *
	 * @param source
	 * @param target
	 * @param rssi
	 *
	 * @return Edge pointer
	 */
	Edge* addOppositeEdge(stone_id_t source, stone_id_t target, int8_t rssi);


	/**
	 * @brief Add a triangle to the triangleList
	 *
	 * @param Triangle*
	 * @return Triangle pointer
	 */
	Triangle* addTriangle(Edge* baseEdge, Edge* adjEdge, Edge* oppositeEdge);

	/**
	 * @brief Create a Edge With target surNode
	 *
	 * @param surNode
	 * @return cs_ret_code_t
	 */
	cs_ret_code_t createEdgeWith(sur_node_t& target);

	/**
	 * @brief Corrects the base position of the triangle based on the angle of the triangle
	 * 
	 * @param altiX 
	 * @param self_angle 
	 * @return float 
	 */
	float basePositionCorrection(float altiX, float angle);

	/**
	 * @brief Create a Triangle With baseEdge and adjEdge and oppositeEdge
	 *
	 * @param oppositeEdge
	 */
	void createTriangleWith(Edge* oppositeEdge);

	/**
	 * @brief send request for the rssi of the target crownstone from the stoneID
	 * 
	 * @param askId 
	 * @param searchId 
	 * @return cs_ret_code_t 
	 */
	cs_ret_code_t requestNodeSearch(stone_id_t askId, stone_id_t searchId);

	/**
	 * @brief handle the node requests messages
	 * @return cs_ret_code_t 
	 */
	cs_ret_code_t onNodeRequest(MeshMsgEvent& meshMsg);

	/**
	 * @brief send request for the altitude of the target crownstone from the stoneID
	 * 
	 * @param askId 
	 * @param searchId 
	 * @return cs_ret_code_t 
	 */
	cs_ret_code_t requestAltitude(stone_id_t askId, stone_id_t base_other, stone_id_t searchId);

	/**
	 * @brief Handles the altitude request
	 * 
	 * @param meshMsg 
	 * @return cs_ret_code_t 
	 */
	cs_ret_code_t onAltitudeRequest(MeshMsgEvent& meshMsg);

	/**
	 * @brief Calculates the angle between two triangles with the same base edge
	 * 
	 * @param defaultOtherNode 
	 * @param otherNode 
	 * @param base_altiX 
	 * @param base_altiH 
	 * @param altiX 
	 * @param altiH 
	 * @return float 
	 */
	float calculateMapAngle(stone_id_t defaultOtherNode, stone_id_t otherNode, float base_altiX, float base_altiH, float altiX, float altiH);
};
