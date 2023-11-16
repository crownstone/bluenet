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
#include <util/cs_math.h>

/******************************************   Edge   *********************************************/

/**
 * @brief Compare two edges.
 *
 * @param other
 * @return true
 * @return false
 */
bool Edge::compare(const Edge& other) const {
	return source == other.source && target == other.target;
}

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
	if (div < 0) {
		div = 0;
	}
	return std::sqrt(div);
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
	if (p < -1.0) {
		p = -1.0;
	}
	if (p > 1.0) {
		p = 1.0;
	}
	return std::acos(p) * (180 / M_PI);
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

	if (basePositionX < 0) {
		basePositionX = 0.0;
	}

	return basePositionX;
}

/************************************   MeshTopologyResearch   ***********************************/

MeshTopologyResearch::MeshTopologyResearch()
		: routine([this]() {
			LOGi("Routine started");

			unsigned long long int a = 0, b = 1, c, i;
			for (i = 0; i < 100000000; ++i) {
				c = a + b;
				a = b;
				b = c;
			}

			LOGi("Routine done scramble=%d", c % 2 == 0);

			return 100;
		}) {}

void MeshTopologyResearch::init() {
	LOGi("init");
	TYPIFY(CONFIG_CROWNSTONE_ID) state_id;
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &state_id, sizeof(state_id));

	listen();
}
