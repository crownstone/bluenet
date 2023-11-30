/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2023
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <ble/cs_Nordic.h>
#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <localisation/cs_MeshTopologyResearch.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_RssiAndChannel.h>
#include <storage/cs_State.h>
#include <uart/cs_UartHandler.h>
#include <util/cs_Utils.h>

#define LOGresearchInfo LOGi
#define LOGresearchDebug LOGvv
#define LOGresearchVerbose LOGvv

/************************************   Transformations   ****************************************/

// TODO: in math file later maybe

float rssiToDistance(int8_t rssi) {
	return std::pow(10.0, (-69.1 - rssi) / 10.0 * 2.05);
}

float* roll(float phi) {
	phi           = M_PI * phi / 180.0;
	float* result = new float[9]{1, 0, 0, 0, std::cos(phi), -std::sin(phi), 0, std::sin(phi), std::cos(phi)};
	return result;
}

float* pitch(float theta) {
	theta         = M_PI * theta / 180.0;
	float* result = new float[9]{std::cos(theta), 0, std::sin(theta), 0, 1, 0, -std::sin(theta), 0, std::cos(theta)};
	return result;
}

float* yaw(float psi) {
	psi           = M_PI * psi / 180.0;
	float* result = new float[9]{std::cos(psi), -std::sin(psi), 0, std::sin(psi), std::cos(psi), 0, 0, 0, 1};
	return result;
}

float* mirrorX() {
	return new float[9]{-1, 0, 0, 0, 1, 0, 0, 0, 1};
}

float* mirrorY() {
	return new float[9]{1, 0, 0, 0, -1, 0, 0, 0, 1};
}

float* mirrorZ() {
	return new float[9]{1, 0, 0, 0, 1, 0, 0, 0, -1};
}

float* matrix3_multiply(const float* matrix, const float* vectMatrix, size_t size) {
	float* result = new float[size]();  // Initialize to zero

	if (size == 3) {
		for (size_t i = 0; i < 3; ++i) {
			for (size_t j = 0; j < 3; ++j) {
				result[i] += matrix[i * 3 + j] * vectMatrix[j];
			}
		}
	}
	if (size == 9) {
		for (size_t i = 0; i < 3; ++i) {
			for (size_t j = 0; j < 3; ++j) {
				for (size_t k = 0; k < 3; ++k) {
					result[i * 3 + j] += matrix[i * 3 + k] * vectMatrix[k * 3 + j];
				}
			}
		}
	}
	return result;
}

// tested
// float testr[9] = {1.0, 1.0, 1.0,    1.0, 1.0, 0,   0, 1.0, 1.0};
// print all 9 values of the array
// float* test = matrix3_multiply(roll(25), testr, 3);
// LOGresearchInfo("Array %.3f,%.3f,%.3f",test[0],test[1],test[2]);
// LOGresearchInfo("Array %.3f,%.3f,%.3f :: %.3f,%.3f,%.3f ::
// %.3f,%.3f,%.3f",test[0],test[1],test[2],test[3],test[4],test[5],test[6],test[7],test[8]);

/******************************************   Edge   *********************************************/

Edge::Edge(stone_id_t source, stone_id_t target, int8_t rssi) : source(source), target(target), rssi(rssi) {
	distance = rssiToDistance(rssi);
}

/**
 * @brief Compare two edges.
 *
 * @param other
 * @return true
 * @return false
 */
bool Edge::compare(const Edge* other) const {
	if (other == nullptr) {
		return false;
	}
	else {
		return (source == other->source && target == other->target)
			   || (source == other->target && target == other->source);
	}
}

// Edge* edge1 = new Edge(0,1,-75);
// Edge* edge2 = new Edge(0,2,-75);
// Edge* edge3 = new Edge(1,2,-75);
// LOGresearchInfo("Distance edge1 = %.3f", edge1->distance);
// LOGresearchInfo("Distance edge2 = %.3f", edge2->distance);
// LOGresearchInfo("Distance edge3 = %.3f", edge3->distance);
// Edge* dupl = new Edge(1,0,-75);
// if ( edge1->compare(dupl)) {
// 	LOGresearchInfo("TRUE");
// } else {
// 	LOGresearchInfo("FALSE");
// }

/*****************************************  Triangle  ********************************************/

/**
 * @brief Calculates the area of the triangle and returns it.
 * @return float
 */
float Triangle::getArea() {
	float a   = opposite_edge->distance;
	float b   = adj_edge->distance;
	float c   = base_edge->distance;

	float s   = (a + b + c) / 2;
	float div = s * (s - a) * (s - b) * (s - c);

	return std::sqrt(CsMath::max(div, 0));
}

/**
 * @brief Calculates the angle of the triangle and returns it.
 *
 * @return float
 */
float Triangle::getAngle() {
	float a = opposite_edge->distance;
	float b = adj_edge->distance;
	float c = base_edge->distance;

	float p = (std::pow(b, 2) + std::pow(c, 2) - std::pow(a, 2)) / (2 * b * c);

	return std::acos(CsMath::clamp(p, -1, 1)) * (180 / M_PI);
}

/**
 * @brief Calculates the altitude of the triangle from the current perspective of the node and returns it.
 * @return float
 */
float Triangle::getAltitude() {
	float area = getArea();
	return (2.0 * area) / opposite_edge->distance;
}

/**
 * @brief Calculates the position on the base edge this triangle from the current perspective of the node with
 * pythagoras. It returns this length from this position to the target node.
 *
 * @param targetID
 * @return float
 */
float Triangle::getAlitudeBasePositionTo(stone_id_t targetID) {
	float altitude      = getAltitude();
	float hASquared     = altitude * altitude;
	float basePositionX = 0.0;

	if (base_edge->target == targetID) {
		basePositionX = std::pow(base_edge->distance, 2) - hASquared;
	}
	if (adj_edge->target == targetID) {
		basePositionX = std::pow(adj_edge->distance, 2) - hASquared;
	}

	return std::sqrt(CsMath::max(basePositionX, 0));
}

/**
 * @brief Returns the third node of the triangle based on the base edge.
 * Base edge is alway self->otherNode
 *
 * @param baseEdge
 * @return stone_id_t
 */
stone_id_t Triangle::getThirdNode(Edge& baseEdge) {
	return getOtherBase(baseEdge)->target;
}

/**
 * @brief Get the Other outgoing base edge
 *
 * @param baseEdge
 * @return Edge pointer
 */
Edge* Triangle::getOtherBase(Edge& baseEdge) {
	if (base_edge->compare(&baseEdge)) {
		return adj_edge;
	}
	return base_edge;
}

/**
 * @brief Get a specific edge from the triangle.
 *
 * @param source
 * @param target
 * @return Edge pointer
 */
Edge* Triangle::getEdge(stone_id_t source, stone_id_t target) {
	Edge* test = new Edge(source, target, 0);
	if (base_edge->compare(test)) {
		delete test;
		return base_edge;
	}
	if (adj_edge->compare(test)) {
		delete test;
		return adj_edge;
	}
	if (opposite_edge->compare(test)) {
		delete test;
		return opposite_edge;
	}
	delete test;
	return nullptr;
}

// Edge* edge1 = new Edge(0,1,-75);
// Edge* edge2 = new Edge(0,2,-75);
// Edge* edge3 = new Edge(1,2,-75);

// Triangle* triangle = new Triangle(edge1,edge2,edge3);

// LOGresearchInfo("Area = %.3f",triangle->getArea());
// LOGresearchInfo("Angle = %.3f",triangle->getAngle());
// LOGresearchInfo("Altitude = %.3f",triangle->getAltitude());
// LOGresearchInfo("AltitudeBasePositionTo 1= %.3f",triangle->getAlitudeBasePositionTo(1));
// LOGresearchInfo("AltitudeBasePositionTo 2= %.3f",triangle->getAlitudeBasePositionTo(2));

// LOGresearchInfo("Edge 1 distance = %.3f",(triangle->getEdge(0,1))->distance);
// LOGresearchInfo("Edge 2 distance = %.3f",(triangle->getEdge(0,2))->distance);
// LOGresearchInfo("Edge 3 distance = %.3f",(triangle->getEdge(1,2))->distance);

// LOGresearchInfo("Other base edge (not 0-1)=
// %d-%d",(triangle->getOtherBase(*edge1))->source,(triangle->getOtherBase(*edge1))->target); LOGresearchInfo("Third
// node = %d",triangle->getThirdNode(*edge1));

/************************************   MeshTopologyResearch   ***********************************/

MeshTopologyResearch::MeshTopologyResearch() : routine([this]() { return stateFunc(); }) {}

cs_ret_code_t MeshTopologyResearch::init() {

	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_myId, sizeof(_myId));

	surNodeList = new (std::nothrow) sur_node_t[MAX_SURROUNDING];
	if (surNodeList == nullptr) {
		LOGresearchInfo("No space for surNodeList");
		return ERR_NO_SPACE;
	}

	edgeList = new (std::nothrow) Edge[MAX_EDGES];
	if (edgeList == nullptr) {
		LOGresearchInfo("No space for edgeList");
		return ERR_NO_SPACE;
	}

	oppositeEdgeList = new (std::nothrow) Edge[MAX_TRIANGLES];
	if (oppositeEdgeList == nullptr) {
		LOGresearchInfo("No space for oppositeEdgeList");
		return ERR_NO_SPACE;
	}

	trianglesList = new (std::nothrow) Triangle[MAX_TRIANGLES];
	if (trianglesList == nullptr) {
		LOGresearchInfo("No space for trianglesList");
		return ERR_NO_SPACE;
	}

	reset();
	listen();

	return ERR_SUCCESS;
}

void MeshTopologyResearch::reset() {
	LOGresearchInfo("Full Topology Reset");

	// Reset all counters.
	_surNodeCount      = 0;
	_edgeCount         = 0;
	_oppositeEdgeCount = 0;
	_triangleCount     = 0;
}

/**
 * @brief Print all surrounding nodes
 */
void MeshTopologyResearch::printSurNodes() {
	LOGresearchInfo("Surrounding nodes (%d):", _surNodeCount);
	for (int i = 0; i < _surNodeCount; ++i) {
		LOGresearchInfo(
				"id=(%u) mean=[%i]=> rssi=[%i, %i, %i], counter=[%u], secondsAgo=%u",
				surNodeList[i].id,
				surNodeList[i].rollingAverageRssi,
				surNodeList[i].rssiChannel37,
				surNodeList[i].rssiChannel38,
				surNodeList[i].rssiChannel39,
				surNodeList[i].count,
				surNodeList[i].lastSeenSecondsAgo);
	}
}

/**
 * @brief Print all edges
 */
void MeshTopologyResearch::printEdges() {
	LOGresearchInfo("Edges (%d):", _edgeCount);
	for (int i = 0; i < _edgeCount; ++i) {
		LOGresearchInfo(
				"source=%u, target=%u, rssi=%i, distance=%.3f",
				edgeList[i].source,
				edgeList[i].target,
				edgeList[i].rssi,
				edgeList[i].distance);
	}
}

/**
 * @brief Print all triangles
 */
void MeshTopologyResearch::printTriangles() {
	LOGresearchInfo("Triangles: (%d)", _triangleCount);
	for (int i = 0; i < MAX_TRIANGLES; ++i) {
		LOGresearchInfo(
				"id=%u, area=%.3f, angle=%.3f, altitude=%.3f",
				trianglesList[i].id,
				trianglesList[i].getArea(),
				trianglesList[i].getAngle(),
				trianglesList[i].getAltitude());
	}
}

/**
 * @brief States of the federated topolgoy algorithm and transitions conditions
 *
 * @return int: returns how many ticks the next routine will be called
 */
int MeshTopologyResearch::stateFunc() {

	switch (state) {
		case TopologyDiscoveryState::SCAN_FOR_NEIGHBORS: {
			addSurNode(1, -40, 37);
			addSurNode(2, -40, 37);
			addSurNode(3, -50, 37);
			addSurNode(4, -50, 37);

			printSurNodes();

			if (SCAN_FOR_NEIGHBORS_FINISHED) {
				state = TopologyDiscoveryState::BUILD_EDGES;
			}
			break;
		}

		case TopologyDiscoveryState::BUILD_EDGES: {
			// waiting state, request surNode to make edges
			addEdge(1, -40);
			addEdge(2, -40);
			addEdge(3, -50);
			addEdge(4, -50);

			printEdges();
			// after two edges are made, try make opposite edge
			addOppositeEdge(1,2,-40);

			if (test == false){
				test = true;
			} else {
				addOppositeEdge(1,3,-50);
				test = false;
			}

			// For each opposite edge a triangle can be made

			// if opposite edge is larger than current triangle count, go to triangle state
			if (_oppositeEdgeCount > _triangleCount) {
				state = TopologyDiscoveryState::BUILD_TRIANGLES;
			}
			break;
		}

		case TopologyDiscoveryState::BUILD_TRIANGLES: {
			LOGresearchInfo("Build Triangles");
			
			while (_oppositeEdgeCount > _triangleCount){
				createTriangleWith(&oppositeEdgeList[_triangleCount]);
			}
			// process the opposite edges and make triangles

			if (_triangleCount == MAX_TRIANGLES) {
				state = TopologyDiscoveryState::BUILD_TOPOLOGY;
			} else {
				state = TopologyDiscoveryState::BUILD_EDGES;
			}
			break;
		}

			// Zo niet? Blijf in je waiting state. Zo wel? Ga naar je computation state. Als je in je waiting state dan
			// ook een soort van "timer" bouwt dan kun je na een bepaalde tijd weer terug naar de sending state - zo
			// probeer je te mitigeren dat sommige messages niet altijd aankomen case

		case TopologyDiscoveryState::BUILD_TOPOLOGY: { 
			// make adjacent triangles, so they can be mapped, just 1 map is needed with the best edge as base
			// or do it for each edge, and compare/rotate the coordmaps to get final one
			printTriangles();

			state = TopologyDiscoveryState::TOPOLOGY_DONE;
			break; 
		}

		case TopologyDiscoveryState::TOPOLOGY_DONE: {
			// send topology coords to the local HUB, coordinate map only or more?

			state = TopologyDiscoveryState::LOCALISATION;
			break;
		}

		case TopologyDiscoveryState::LOCALISATION: {
			// localise the tracked device inside a tetrahedron
			if(test == false){
				LOGresearchInfo("Localisation process started");
			}
			break;
		}

		default: {
			LOGe("Unhandeled state: %d", state); 
			break;
		} 
	}

	return Coroutine::delayMs(5000);
}

/**
 * @brief Handles all events
 *
 * @param evt
 */
void MeshTopologyResearch::handleEvent(event_t& evt) {
	routine.handleEvent(evt);

	switch (evt.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto packet = CS_TYPE_CAST(EVT_RECV_MESH_MSG, evt.data);
			onMeshMsg(*packet, evt.result);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			auto packet = CS_TYPE_CAST(EVT_TICK, evt.data);
			if (*packet % (1000 / TICK_INTERVAL_MS) == 0) {
				onTickSecond();
			}
			break;
		}
		case CS_TYPE::CMD_MESH_TOPO_RESET: {
			reset();
			evt.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::CMD_MESH_TOPO_GET_RSSI: {
			auto packet = CS_TYPE_CAST(CMD_MESH_TOPO_GET_RSSI, evt.data);
			getRssi(*packet, evt.result);
			break;
		}
		default: break;
	}
}

/**
 * @brief Handles all the Mesh messages
 *
 * @param id
 * @param packet
 */
void MeshTopologyResearch::onMeshMsg(MeshMsgEvent& packet, cs_result_t& result) {
	if (packet.type == CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI) {
		// all incomming RSSI messages can be used as hearbeat for the lastseen counter of a sur_node
		cs_mesh_model_msg_neighbour_rssi_t payload = packet.getPacket<CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI>();
		onNeighbourRssi(packet.srcStoneId, payload);
	}
	if (packet.isMaybeRelayed) {
		return;
	}
	if (packet.type == CS_MESH_MODEL_TYPE_NODE_REQUEST) {
		onNodeRequest(packet);
		return;
	}
	if (packet.type == CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST) {
		onAltitudeRequest(packet);
		return;
	}

	// // Localisation in a tetrahedron, 3 nodes are asked for the RSSI of the Tracked device
	// if (packet.type == CS_MESH_MODEL_TYPE_TRACKED_REQUEST) {
	// 	onTrackedRequest(packet); // RSSI values to tracked device should be stored, other function will use these
	// // values to determine the location 	
	// 	return;
	// }

	// every packet that is received should be added as a surrounding Node, to get a starting list of surNodes
	// TODO: uncomment this
	addSurNode(packet.srcStoneId, packet.rssi, packet.channel);
}

/**
 * @brief Tick Timer scheduler: remove surNodes when they are not seen for a while
 *
 */
void MeshTopologyResearch::onTickSecond() {
	[[maybe_unused]] bool change = false;
	for (uint8_t i = 0; i < _surNodeCount; /**/) {
		surNodeList[i].lastSeenSecondsAgo++;
		if (surNodeList[i].lastSeenSecondsAgo == TIMEOUT_SECONDS) {
			//TODO: uncomment this
			// change = true;
			// // Remove item, by shifting all items after this item.
			// _surNodeCount--;
			// for (uint8_t j = i; j < _surNodeCount; ++j) {
			// 	_neighbours[j] = _neighbours[j + 1];
			// }
		}
		else {
			i++;
		}
	}
	if (change) {
		sortSurNodeList();
	}
	// if (++_nextPrint == 10) {
	// 	printSurNodes();
	// 	_nextPrint = 0;
	// }
	if (++_sendNeighbours == TIMEOUT_SEND_NEIGHBOURS) {
		SCAN_FOR_NEIGHBORS_FINISHED = true;
		_sendNeighbours             = 0;
	}
}

//***************************************************************************************************************//
//********************************************    Functions    **************************************************//
//***************************************************************************************************************//

/**
 * @brief Construct a new Neighbour-RSSI-message via Mesh
 *
 * @param surNode
 */
void MeshTopologyResearch::sendNeighbourRSSIviaMesh(sur_node_t& surNode) {

	// Sent RSSI of neighbour over the mesh
	cs_mesh_model_msg_neighbour_rssi_t meshPayload = {
			.type               = 0,
			.neighbourId        = surNode.id,
			.rssiChannel37      = surNode.rssiChannel37,
			.rssiChannel38      = surNode.rssiChannel38,
			.rssiChannel39      = surNode.rssiChannel39,
			.lastSeenSecondsAgo = surNode.lastSeenSecondsAgo,
			.counter            = _msgCount++};

	TYPIFY(CMD_SEND_MESH_MSG) meshMsg;
	meshMsg.type                   = CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI;
	meshMsg.reliability            = CS_MESH_RELIABILITY_LOWEST;
	meshMsg.urgency                = CS_MESH_URGENCY_LOW;
	meshMsg.flags.flags.doNotRelay = false;
	meshMsg.payload                = reinterpret_cast<uint8_t*>(&meshPayload);
	meshMsg.size                   = sizeof(meshPayload);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
}

/**
 * @brief Get the Mean of the RSSI packet
 *
 * @param packet
 * @return uint8_t
 */
uint8_t MeshTopologyResearch::getMean(cs_mesh_model_msg_neighbour_rssi_t& packet) {
	int8_t meanRSSI = 0;
	uint8_t count   = 0;
	if (packet.rssiChannel37) {
		meanRSSI += packet.rssiChannel37;
		count++;
	}
	if (packet.rssiChannel38) {
		meanRSSI += packet.rssiChannel38;
		count++;
	}
	if (packet.rssiChannel39) {
		meanRSSI += packet.rssiChannel39;
		count++;
	}
	if (count) {
		meanRSSI = meanRSSI / count;
	}
	return meanRSSI;
}

/**
 * @brief Log the neighbourRSSI messages send over the MESH
 *
 * @param OGsenderID
 * @param packet
 */
void MeshTopologyResearch::onNeighbourRssi(stone_id_t OGsenderID, cs_mesh_model_msg_neighbour_rssi_t& packet) {
	LOGresearchInfo("Stone %d sees %d, mean RSSI[%d]", OGsenderID, packet.neighbourId, getMean(packet));
	sendRssiToUart(OGsenderID, packet);
}

/**
 * @brief Find a surrounding node in the surNodeList.
 *
 * @param id
 * @return uint8_t
 */
uint8_t MeshTopologyResearch::findSurNode(stone_id_t id) {
	for (uint8_t index = 0; index < _surNodeCount; ++index) {
		if (surNodeList[index].id == id) {
			return index;
		}
	}
	return INDEX_NOT_FOUND;
}

/**
 * @brief Find an edge in the edgeList.
 *
 * @param id
 * @return uint8_t
 */
uint8_t MeshTopologyResearch::findEdge(stone_id_t id) {
	for (uint8_t index = 0; index < _edgeCount; ++index) {
		if (edgeList[index].target == id) {
			return index;
		}
	}
	return INDEX_NOT_FOUND;
}

/**
 * @brief Find an opposite edge in the oppositeEdgeList.
 *
 * @param id
 * @return uint8_t
 */
uint8_t MeshTopologyResearch::findOppositeEdge(stone_id_t id) {
	for (uint8_t index = 0; index < _oppositeEdgeCount; ++index) {
		if (oppositeEdgeList[index].target == id) {
			return index;
		}
	}
	return INDEX_NOT_FOUND;
}

/**
 * @brief Find a triangle in the trianglesList.
 *
 * @param id
 * @return uint8_t
 */
uint8_t MeshTopologyResearch::findTriangle(stone_id_t edge_src, stone_id_t edge_dst) {
	for (uint8_t index = 0; index < _triangleCount; ++index) {
		if (trianglesList[index].getEdge(edge_src, edge_dst) != nullptr) {
			return index;
		}
	}
	return INDEX_NOT_FOUND;
}

/**
 * @brief clears the RSSI stats values to RSSI_INIT and zero
 *
 * @param node
 */
void MeshTopologyResearch::clearSurNodeRSSIinfo(sur_node_t& node) {
	node.rssiChannel37      = RSSI_INIT;
	node.rssiChannel38      = RSSI_INIT;
	node.rssiChannel39      = RSSI_INIT;
	node.rollingAverageRssi = RSSI_INIT;
	node.count              = 0;
}

/**
 * @brief Add a Node to the surNodeList, if not addressed to self or RSSI > -85
 *
 * @param source
 * @param target
 * @param rssi
 */
void MeshTopologyResearch::addSurNode(stone_id_t id, int8_t rssi, uint8_t channel) {
	if (id == 0 || id == _myId || rssi < -70) {
		return;
	}

	uint8_t index = findSurNode(id);
	if (index == INDEX_NOT_FOUND) {
		if (_surNodeCount < MAX_SURROUNDING) {
			clearSurNodeRSSIinfo(surNodeList[_surNodeCount]);
			updateSurNode(surNodeList[_surNodeCount], id, rssi, channel);
			_surNodeCount++;
			if (_surNodeCount == MAX_SURROUNDING) {
				sortSurNodeList();
			}
		}
		else {
			if (rssi > surNodeList[MAX_SURROUNDING - 1].rollingAverageRssi) {
				clearSurNodeRSSIinfo(surNodeList[MAX_SURROUNDING - 1]);
				updateSurNode(surNodeList[MAX_SURROUNDING - 1], id, rssi, channel);
				sortSurNodeList();
			}
		}
	}
	else {
		updateSurNode(surNodeList[index], id, rssi, channel);
	}
}

/**
 * @brief Sort the surNodeList on RSSI
 */
void MeshTopologyResearch::sortSurNodeList() {
	std::sort(surNodeList, surNodeList + _surNodeCount, [](const sur_node_t& a, const sur_node_t& b) {
		return a.rollingAverageRssi > b.rollingAverageRssi;
	});
}

/**
 * @brief Update a surrounding node in the surNodeList or add a new one to the list
 * (helperfunction of addSurNode)
 *
 * @param node
 * @param id
 * @param rssi
 */
void MeshTopologyResearch::updateSurNode(sur_node_t& node, stone_id_t id, int8_t rssi, uint8_t channel) {
	node.id = id;
	switch (channel) {
		case 37: {
			node.rssiChannel37 = rssi;
			break;
		}
		case 38: {
			node.rssiChannel38 = rssi;
			break;
		}
		case 39: {
			node.rssiChannel39 = rssi;
			break;
		}
	}
	node.rollingAverageRssi = (node.rollingAverageRssi * node.count + rssi) / (node.count + 1);
	node.count++;
	node.lastSeenSecondsAgo = 0;
}

/**
 * @brief Send a neighbour RSSI message to the UART
 *
 * @param receiverId
 * @param packet
 */
void MeshTopologyResearch::sendRssiToUart(stone_id_t receiverId, cs_mesh_model_msg_neighbour_rssi_t& packet) {
	LOGresearchInfo("sendRssiToUart receiverId=%u senderId=%u", receiverId, packet.neighbourId);
	mesh_topology_neighbour_rssi_uart_t uartMsg = {
			.type               = 0,
			.receiverId         = receiverId,
			.senderId           = packet.neighbourId,
			.rssiChannel37      = packet.rssiChannel37,
			.rssiChannel38      = packet.rssiChannel38,
			.rssiChannel39      = packet.rssiChannel39,
			.lastSeenSecondsAgo = packet.lastSeenSecondsAgo,
			.msgNumber          = packet.counter};
	UartHandler::getInstance().writeMsg(
			UART_OPCODE_TX_NEIGHBOUR_RSSI, reinterpret_cast<uint8_t*>(&uartMsg), sizeof(uartMsg));
}

/**
 * @brief Copy RSSI of stoneID into a result buffer
 *
 * @param stoneId
 * @param result
 */
void MeshTopologyResearch::getRssi(stone_id_t stoneId, cs_result_t& result) {
	uint8_t index = findSurNode(stoneId);
	if (index == INDEX_NOT_FOUND) {
		result.returnCode = ERR_NOT_FOUND;
		return;
	}

	int8_t rssi = surNodeList[index].rollingAverageRssi;

	if (result.buf.len < sizeof(rssi)) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}
	// Copy to result buf.
	result.buf.data[0] = *reinterpret_cast<uint8_t*>(&rssi);
	result.dataSize    = sizeof(rssi);
	result.returnCode  = ERR_SUCCESS;
}

/**
 * @brief Add a Edge to the EdgeList
 *
 * @param source
 * @param target
 * @param rssi
 */
Edge* MeshTopologyResearch::addEdge(stone_id_t target, int8_t rssi) {
	if (target == 0 || target == _myId) {
		return nullptr;
	}

	uint8_t index = findEdge(target);
	if (index == INDEX_NOT_FOUND) {
		if (_edgeCount < MAX_EDGES) {
			edgeList[_edgeCount].source = _myId;
			edgeList[_edgeCount].target = target;
			edgeList[_edgeCount].rssi   = rssi;
			edgeList[_edgeCount].distance = rssiToDistance(rssi);
			_edgeCount++;
			return &edgeList[_edgeCount - 1];
		}
		else {
			return nullptr;
			LOGresearchInfo("EdgeList FULL");
		}
	}
	else {
		edgeList[index].rssi = (edgeList[index].rssi + rssi) / 2;
		edgeList[index].distance = (edgeList[index].distance + rssiToDistance(rssi)) / 2;
		return &edgeList[index];
	}
}

/**
 * @brief Add an edge to the oppositeEdgelist
 *
 * @param source
 * @param target
 * @param rssi
 */
Edge* MeshTopologyResearch::addOppositeEdge(stone_id_t source, stone_id_t target, int8_t rssi) {
	if (target == 0 || source == 0 || target == _myId || source == _myId) {
		return nullptr;
	}

	uint8_t index = findOppositeEdge(target);
	if (index == INDEX_NOT_FOUND) {
		if (_edgeCount < MAX_TRIANGLES) {
			oppositeEdgeList[_oppositeEdgeCount].source = source;
			oppositeEdgeList[_oppositeEdgeCount].target = target;
			oppositeEdgeList[_oppositeEdgeCount].rssi   = rssi;
			oppositeEdgeList[_oppositeEdgeCount].distance = rssiToDistance(rssi);
			_oppositeEdgeCount++;
			return &oppositeEdgeList[_oppositeEdgeCount - 1];
		}
		else {
			return nullptr;
			LOGresearchInfo("EdgeList FULL");
		}
	}
	else {
		oppositeEdgeList[index].rssi = (oppositeEdgeList[index].rssi + rssi) / 2;
		oppositeEdgeList[index].distance = (oppositeEdgeList[index].distance + rssiToDistance(rssi)) / 2;
		return &oppositeEdgeList[index];
	}
}

/**
 * @brief Add a triangle to the triangleList, only with baseEdge.src == _myId
 *
 * @param baseEdge
 * @param oppositeEdge
 */
Triangle* MeshTopologyResearch::addTriangle(Edge* baseEdge, Edge* adjEdge, Edge* oppositeEdge) {
	if (baseEdge == nullptr || adjEdge == nullptr || oppositeEdge == nullptr || baseEdge->source != _myId) {
		return nullptr;
	}

	uint8_t index = findTriangle(oppositeEdge->source, oppositeEdge->target);
	if (index == INDEX_NOT_FOUND) {
		if (_triangleCount < MAX_TRIANGLES) {
			trianglesList[_triangleCount].base_edge     = baseEdge;
			trianglesList[_triangleCount].adj_edge      = adjEdge;
			trianglesList[_triangleCount].opposite_edge = oppositeEdge;
			_triangleCount++;
			return &trianglesList[_triangleCount - 1];
		}
		else {
			LOGresearchInfo("TriangleList FULL");
			return nullptr;
		}
	}
	else {
		return &trianglesList[index];
	}
}

/**
 * @brief Create a Edge With target surNode
 *
 * @param surNode
 * @return cs_ret_code_t
 */
cs_ret_code_t MeshTopologyResearch::createEdgeWith(sur_node_t& target) {
	uint8_t index = findSurNode(target.id);
	if (index == INDEX_NOT_FOUND) {
		LOGresearchInfo("Edge cant be made with non existing surNode");
		return ERR_NOT_FOUND;
	}
	// send message
	return requestNodeSearch(target.id, _myId);
}

/**
 * @brief Request a node to search for searchId at askID
 *
 * @param askId
 * @param searchId
 * @return cs_ret_code_t
 */
cs_ret_code_t MeshTopologyResearch::requestNodeSearch(stone_id_t askId, stone_id_t searchId) {

	// make node request
	cs_mesh_model_msg_node_request_t request;
	request.targetId = searchId;
	request.rssi     = 0;

	cs_mesh_msg_t meshMsg;
	meshMsg.type                    = CS_MESH_MODEL_TYPE_NODE_REQUEST;
	meshMsg.flags.flags.broadcast   = false;
	meshMsg.flags.flags.acked       = true;
	meshMsg.flags.flags.useKnownIds = false;
	meshMsg.flags.flags.doNotRelay  = true;
	meshMsg.reliability             = 3;  // Low timeout, we expect a result quickly.
	meshMsg.urgency                 = CS_MESH_URGENCY_LOW;
	meshMsg.idCount                 = 1;
	meshMsg.targetIds               = &askId;
	meshMsg.payload                 = reinterpret_cast<uint8_t*>(&request);
	meshMsg.size                    = sizeof(request);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
	if (event.result.returnCode != ERR_SUCCESS) {
		return event.result.returnCode;
	}

	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t MeshTopologyResearch::onNodeRequest(MeshMsgEvent& meshMsg) {
	cs_mesh_model_msg_node_request_t packet = meshMsg.getPacket<CS_MESH_MODEL_TYPE_NODE_REQUEST>();
	LOGresearchInfo("Node request received from %u", meshMsg.srcStoneId);
	switch (meshMsg.isReply) {
		// incomming request
		case false: {
			LOGresearchInfo("Reply to node RSSI request");
			if (meshMsg.reply == nullptr) {
				return ERR_BUFFER_UNASSIGNED;
			}
			if (meshMsg.reply->buf.len < sizeof(cs_mesh_model_msg_node_request_t)) {
				return ERR_BUFFER_TOO_SMALL;
			}

			// find source ID surNode for RSSI sur Node request
			uint8_t index           = findSurNode(packet.targetId);

			auto* replyPacket       = reinterpret_cast<cs_mesh_model_msg_node_request_t*>(meshMsg.reply->buf.data);

			replyPacket->rssi       = index == INDEX_NOT_FOUND ? 0 : surNodeList[index].rollingAverageRssi;
			meshMsg.reply->type     = CS_MESH_MODEL_TYPE_NODE_REQUEST;
			meshMsg.reply->dataSize = sizeof(cs_mesh_model_msg_node_request_t);
			// reply sent
			break;
		}
		// incomming reply
		case true: {
			LOGresearchInfo("Received result Node RSSI from id=%u", meshMsg.srcStoneId);

			// make edge
			// if target is self, make edge with source
			if (packet.targetId == _myId) {
				addEdge(meshMsg.srcStoneId, packet.rssi);
			}
			// if target is other: store somewhre it will be needed for triangle creation
			else {
				// Opposite Edge from one side, store this temporarily
				addOppositeEdge(meshMsg.srcStoneId, packet.targetId, packet.rssi);
			}

			break;
		}
	}
	return ERR_SUCCESS;
}

/**
 * @brief send request for the altitude of the target crownstone from the stoneID
 *
 * @param askId
 * @param searchId
 * @return cs_ret_code_t
 */
cs_ret_code_t MeshTopologyResearch::requestAltitude(stone_id_t askId, stone_id_t base_other, stone_id_t searchId) {

	// make node request
	cs_mesh_model_msg_altitude_request_t request;
	request.baseEdge_other = base_other;
	request.targetID       = searchId;

	cs_mesh_msg_t meshMsg;
	meshMsg.type                    = CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST;
	meshMsg.flags.flags.broadcast   = false;
	meshMsg.flags.flags.acked       = true;
	meshMsg.flags.flags.useKnownIds = false;
	meshMsg.flags.flags.doNotRelay  = true;
	meshMsg.reliability             = 3;  // Low timeout, we expect a result quickly.
	meshMsg.urgency                 = CS_MESH_URGENCY_LOW;
	meshMsg.idCount                 = 1;
	meshMsg.targetIds               = &askId;
	meshMsg.payload                 = reinterpret_cast<uint8_t*>(&request);
	meshMsg.size                    = sizeof(request);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
	if (event.result.returnCode != ERR_SUCCESS) {
		return event.result.returnCode;
	}

	return ERR_WAIT_FOR_SUCCESS;
}

/**
 * @brief Handles the altitude request
 *
 * @param meshMsg
 * @return cs_ret_code_t
 */
cs_ret_code_t MeshTopologyResearch::onAltitudeRequest(MeshMsgEvent& meshMsg) {
	cs_mesh_model_msg_altitude_request_t packet = meshMsg.getPacket<CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST>();

	LOGresearchInfo("Altitude request received from %u", meshMsg.srcStoneId);
	switch (meshMsg.isReply) {
		// incomming request
		case false: {
			LOGresearchInfo("Reply to altitude request");
			if (meshMsg.reply == nullptr) {
				return ERR_BUFFER_UNASSIGNED;
			}
			if (meshMsg.reply->buf.len < sizeof(cs_mesh_model_msg_altitude_request_t)) {
				return ERR_BUFFER_TOO_SMALL;
			}

			// find the triangle with the edge (srcStoneId, baseEdge_other)
			uint8_t index         = findTriangle(meshMsg.srcStoneId, packet.baseEdge_other);

			auto* replyPacket     = reinterpret_cast<cs_mesh_model_msg_altitude_request_t*>(meshMsg.reply->buf.data);

			replyPacket->targetID = packet.targetID;
			replyPacket->baseEdge_other = packet.baseEdge_other;
			replyPacket->altitude       = index == INDEX_NOT_FOUND ? -1 : trianglesList[index].getAltitude();
			replyPacket->basePositionX =
					index == INDEX_NOT_FOUND ? -1 : trianglesList[index].getAlitudeBasePositionTo(packet.targetID);
			meshMsg.reply->type     = CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST;
			meshMsg.reply->dataSize = sizeof(cs_mesh_model_msg_altitude_request_t);
			// reply sent
			break;
		}
		// incomming reply
		case true: {
			LOGresearchInfo(
					"Received result altitude from id=%u : altitude(%.3f), baseXposition(%.3f)",
					meshMsg.srcStoneId,
					packet.altitude,
					packet.basePositionX);

			// add altitude information to the adjacent triangle
			LOGresearchInfo("Triangle (%u)(%u)(%u)", _myId, packet.baseEdge_other, meshMsg.srcStoneId);
			LOGresearchInfo("Base edge: (%u)<->(%u)", _myId, packet.baseEdge_other);

			// add to queue or something so it can be used stored in an adjacent triangle struct?!?
			break;
		}
	}
	return ERR_SUCCESS;
}

/**
 * @brief Corrects the base position of the triangle based on the angle of the triangle
 *
 * @param altiX
 * @param self_angle
 * @return float
 */
float MeshTopologyResearch::basePositionCorrection(float altiX, float self_angle) {
	if (self_angle > 90) {
		return -altiX;
	}
	else {
		return altiX;
	}
}

/**
 * @brief Create a Triangle with oppositeEdge src and dst
 * TODO: maybe add something for tempuary triangles
 *
 * @param baseEdge
 * @param adjEdge
 * @param oppositeEdge
 */
void MeshTopologyResearch::createTriangleWith(Edge* oppositeEdge) {
	uint8_t baseDstIndex = findEdge(oppositeEdge->source);
	if (baseDstIndex == INDEX_NOT_FOUND) {
		LOGresearchInfo("Triangle cant be made with non existing surNode");
		return;
	}
	uint8_t adjDstIndex = findEdge(oppositeEdge->target);
	if (adjDstIndex == INDEX_NOT_FOUND) {
		LOGresearchInfo("Triangle cant be made with non existing surNode");
		return;
	}
	addTriangle(&edgeList[baseDstIndex], &edgeList[adjDstIndex], oppositeEdge);
}

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
float MeshTopologyResearch::calculateMapAngle(
		stone_id_t defaultOtherNode,
		stone_id_t otherNode,
		float base_altiX,
		float base_altiH,
		float altiX,
		float altiH) {

	uint8_t index = findTriangle(defaultOtherNode, otherNode);  // first check internal

	if (index == INDEX_NOT_FOUND) {
		// if not found, request the triangle from the other node
		// createTempTriangle(_myId, defaultOtherNode, otherNode);
		return -1;
	}
	float nodeDist    = trianglesList[index].opposite_edge->distance;
	float deltaX      = abs(base_altiX - altiX);
	float pyth        = CsMath::max(0.0f, std::pow(nodeDist, 2) - std::pow(deltaX, 2));

	float translation = sqrt(pyth);
	float a = translation, b = base_altiH, c = altiH;
	if (c == 0) {
		return 0;
	}
	else {
		float p        = CsMath::max(-1.0f, CsMath::min(1.0f, (pow(b, 2) + pow(c, 2) - pow(a, 2)) / (2 * b * c)));
		float mapAngle = std::acos(p) * 180 / M_PI;
		return mapAngle;
	}
}