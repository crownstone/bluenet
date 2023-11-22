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
	return std::pow(10, (-69.1 - rssi) / 10 * 2.05);
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
// LOGresearchInfo("Array %f,%f,%f",test[0],test[1],test[2]);
// LOGresearchInfo("Array %f,%f,%f :: %f,%f,%f ::
// %f,%f,%f",test[0],test[1],test[2],test[3],test[4],test[5],test[6],test[7],test[8]);

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
// LOGresearchInfo("Distance edge1 = %f", edge1->distance);
// LOGresearchInfo("Distance edge2 = %f", edge2->distance);
// LOGresearchInfo("Distance edge3 = %f", edge3->distance);
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

// LOGresearchInfo("Area = %f",triangle->getArea());
// LOGresearchInfo("Angle = %f",triangle->getAngle());
// LOGresearchInfo("Altitude = %f",triangle->getAltitude());
// LOGresearchInfo("AltitudeBasePositionTo 1= %f",triangle->getAlitudeBasePositionTo(1));
// LOGresearchInfo("AltitudeBasePositionTo 2= %f",triangle->getAlitudeBasePositionTo(2));

// LOGresearchInfo("Edge 1 distance = %f",(triangle->getEdge(0,1))->distance);
// LOGresearchInfo("Edge 2 distance = %f",(triangle->getEdge(0,2))->distance);
// LOGresearchInfo("Edge 3 distance = %f",(triangle->getEdge(1,2))->distance);

// LOGresearchInfo("Other base edge (not 0-1)=
// %d-%d",(triangle->getOtherBase(*edge1))->source,(triangle->getOtherBase(*edge1))->target); LOGresearchInfo("Third
// node = %d",triangle->getThirdNode(*edge1));

/************************************   MeshTopologyResearch   ***********************************/

MeshTopologyResearch::MeshTopologyResearch() : routine([this]() { return stateFunc(); }) {}

cs_ret_code_t MeshTopologyResearch::init() {
	reset();
	listen();

	return ERR_SUCCESS;
}

void MeshTopologyResearch::reset() {
	LOGresearchInfo("FULL RESET");

	// Reset all counters.
	_surNodeCount  = 0;
	_edgeCount     = 0;
	_triangleCount = 0;

	// Reset all lists.
	for (int i = 0; i < MAX_SURROUNDING; ++i) {
		surNodeList[i].reset();
	}
	for (int i = 0; i < MAX_EDGES; ++i) {
		edgeList[i].reset();
	}
	for (int i = 0; i < MAX_TRIANGLES; ++i) {
		trianglesList[i].reset();
	}
}

// returns how many ticks the next routine will be called
int MeshTopologyResearch::stateFunc() {

	switch (state) {
		case TopologyDiscoveryState::SCAN_FOR_NEIGHBORS:
			// Mesh topology does this already, maybe we can reuse the neighbor list and mac list
			// by combining and sorting it to a different list of sur_nodes_t
			// only added neighbours with RSSI < 80
			// check if we have enough neighbors?
			// if yes: go to next state
			// if no: don't go to next statue (return  Coroutine::delayMs(1000); )
			{ break; }

		case TopologyDiscoveryState::BUILD_TRIANGLES:
			// Mesh messages: CMD_MESH_TOPO_RS_edgeConfirmation,
			// to sent a message (also take a look at docs/tutorials/ADD_MESH_MSG.md)
			// event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &eventData, sizeof(eventData));
			// EventDispatcher::getInstance().dispatch(cmd);
			{ break; }

		case TopologyDiscoveryState::BUILD_TOPOLOGY:
			// Need some communcation: asking node for triangles, sur_node, altitude information
			// event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &eventData, sizeof(eventData));
			// EventDispatcher::getInstance().dispatch(cmd);
			{ break; }

		case TopologyDiscoveryState::TOPOLOGY_DONE: {
			// send topology coords to the local HUB
			break;
		}

		default: LOGe("Unhandeled state: %d", state); break;
	}

	return Coroutine::delayMs(1000);
}

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
				// onTickSecond(); something todo with lastseen counterdown counter
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

//***************************************************************************************************************//

/**
 * @brief processes all the neighbour RSSI messages of the mesh
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
		// cs_mesh_model_msg_node_request_t payload = packet.getPacket<CS_MESH_MODEL_TYPE_NODE_REQUEST>();
		// onNodeRequest(packet.srcStoneId, payload);
		return;
	}
	if (packet.type == CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST) {
		// cs_mesh_model_msg_altitude_request_t payload = packet.getPacket<CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST>();
		// onAltitudeRequest(packet.srcStoneId, payload);
		return;
	}
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
 * @brief 
 * 
 * @param id 
 * @param packet 
 * @param channel 
 */
void MeshTopologyResearch::onNeighbourRssi(stone_id_t id, cs_mesh_model_msg_neighbour_rssi_t& packet) {
	// Add surNode   
	addSurNode(id, getMean(packet));
	// Send to UART.
	sendRssiToUart(id, packet);
	// LOGi("Stone %d sees %d, [%d, %d, %d]", id, packet.neighbourId, packet.rssiChannel37, packet.rssiChannel38,
	// packet.rssiChannel39);
}

/**
 * @brief Send RSSI to uart
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
 * @brief Find a surrounding node in the surNodeList.
 *
 * @param id
 * @return uint8_t
 */
uint8_t MeshTopologyResearch::findSurNode(stone_id_t id) {
	for (uint8_t index = 0; index < _surNodeCount; ++index) {
		if (surNodeList[index].has_value() && surNodeList[index].value().id == id) {
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
		if (edgeList[index].has_value() && edgeList[index].value().target == id) {
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
uint8_t MeshTopologyResearch::findTriangle(triangle_id_t id) {
	for (uint8_t index = 0; index < _triangleCount; ++index) {
		if (trianglesList[index].has_value() && trianglesList[index].value().id == id) {
			return index;
		}
	}
	return INDEX_NOT_FOUND;
}

/**
 * @brief Add a Node to the surNodeList, if not itself or RSSI > -85
 *
 * @param source
 * @param target
 * @param rssi
 */
void MeshTopologyResearch::addSurNode(stone_id_t id, int8_t rssi) {
	if (id == 0 || rssi > -85) {
		return;
	}

	uint8_t index = findSurNode(id);
	if (index == INDEX_NOT_FOUND) {
		if (_surNodeCount < MAX_SURROUNDING) {
			updateSurNode(surNodeList[_surNodeCount].value(), id, rssi);
			_surNodeCount++;
		}
		else {
			LOGe("Can't add id=%u", id);
		}
	}
	else {
		updateSurNode(surNodeList[index].value(), id, rssi);
	}
}
/**
 * @brief Update a surrounding node in the surNodeList or add a new one to the list
 * (helperfunction of addSurNode)
 *
 * @param node
 * @param id
 * @param rssi
 */
void MeshTopologyResearch::updateSurNode(sur_node_t& node, stone_id_t id, int8_t rssi) {
	node.id                 = id;
	node.rollingAverageRssi = (node.rollingAverageRssi * node.count + rssi) / (node.count + 1);
	node.count++;
	node.lastSeenSecondsAgo = 0;
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

	int8_t rssi = surNodeList[index].value().rollingAverageRssi;

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
void MeshTopologyResearch::addEdge(stone_id_t source, stone_id_t target, int8_t rssi) {
	if (target == 0) {
		return;
	}

	uint8_t index = findEdge(target);
	if (index == INDEX_NOT_FOUND) {
		if (_edgeCount < MAX_EDGES) {
			edgeList[_edgeCount] = Edge(source, target, rssi);
			_edgeCount++;
		}
		else {
			LOGresearchInfo("EdgeList FULL");
		}
	}
	else {
		// nothing
		return;
	}
}