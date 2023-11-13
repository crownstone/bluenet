/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <localisation/cs_MeshTopologyResearch.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>

/************************************   Transformations   ****************************************/

// TODO: in math file later maybe

	float rssToDistance(int8_t rssi) {
    	return std::pow(10, (-69.1-rssi) / 10*2.05);
	}

	std::vector<float> roll(float phi) {
        phi = M_PI * phi / 180.0;
        return {1, 0, 0,
                0, std::cos(phi), -std::sin(phi),
                0, std::sin(phi), std::cos(phi)};
    }

    std::vector<float> pitch(float theta) {
        theta = M_PI * theta / 180.0;
        return {std::cos(theta), 0, std::sin(theta),
                0, 1, 0,
                -std::sin(theta), 0, std::cos(theta)};
    }

    std::vector<float> yaw(float psi) {
        psi = M_PI * psi / 180.0;
        return {std::cos(psi), -std::sin(psi), 0,
                std::sin(psi), std::cos(psi), 0,
                0, 0, 1};
    }

	std::vector<float> matrix3_multiply(const std::vector<float>& matrix, const std::vector<float>& vectMatrix) {
		size_t size = vectMatrix.size();
		std::vector<float> result(size, 0.0);

		if(size == 3){
			for (size_t i = 0; i < 3; ++i) {
				for (size_t j = 0; j < 3; ++j) {
					result[i] += matrix[i * 3 + j] * vectMatrix[j];
				}
			}
		}
		if(size == 9){
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

/******************************************   Edge   *********************************************/

/**
 * @brief Construct a new Edge:: Edge object
 * 
 * @param source 
 * @param target 
 * @param rssi 
 */
Edge::Edge(stone_id_t source, stone_id_t target, int8_t rssi) : source(source), target(target), rssi(rssi) {}

/**
 * @brief Compare two edges.
 * 
 * @param other 
 * @return true 
 * @return false 
 */
bool Edge::compare(const Edge &other) const {
	return source == other.source && target == other.target;
}

/*****************************************  Triangle  ********************************************/

/**
 * @brief Construct a new Triangle:: Triangle object
 * 
 * @param base_edge 
 * @param adj_edge 
 * @param opposite_edge 
 */
Triangle::Triangle(Edge *base_edge, Edge *adj_edge, Edge *opposite_edge) : base_edge(base_edge), adj_edge(adj_edge), opposite_edge(opposite_edge) {}

/**
 * @brief Calculates the area of the triangle and returns it.
 * @return float 
 */
float Triangle::getArea() {
	float a = opposite_edge->distance;
    float b = adj_edge->distance;
    float c = base_edge->distance;

    float s = (a + b + c) / 2;
    float div = s * (s - a) * (s - b) * (s - c);

    return std::sqrt(CsMath::max(div,0));
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

	float p = (std::pow(b,2) + std::pow(c,2) - std::pow(a,2)) / (2 * b * c);

	return std::acos(CsMath::clamp(p,-1,1)) * (180 / M_PI);
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
 * @brief Calculates the position on the base edge this triangle from the current perspective of the node with pythagoras. 
 * It returns this length from this position to the target node.
 * 
 * @param targetID 
 * @return float 
 */
float Triangle::getAlitudeBasePositionTo(stone_id_t targetID) {
 	float altitude = getAltitude();
    float hASquared = altitude*altitude;
	float basePositionX = 0.0;

	if(base_edge->target == targetID){
		basePositionX = std::pow(base_edge->distance,2) - hASquared;
	}
	if(adj_edge->target == targetID){
		basePositionX = std::pow(adj_edge->distance,2) - hASquared;
	}

    return CsMath::max(basePositionX,0);
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
	if(baseEdge.compare(*base_edge)){
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
	if(base_edge->source == source && base_edge->target == target){
		return base_edge;
	}
	if(adj_edge->source == source && adj_edge->target == target){
		return adj_edge;
	}
	if(opposite_edge->source == source && opposite_edge->target == target){
		return opposite_edge;
	}
	return nullptr;
}



/************************************   MeshTopologyResearch   ***********************************/


MeshTopologyResearch::MeshTopologyResearch() {}

void MeshTopologyResearch::init() {
	TYPIFY(CONFIG_CROWNSTONE_ID) state_id;
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &state_id, sizeof(state_id));

	listen();
}
