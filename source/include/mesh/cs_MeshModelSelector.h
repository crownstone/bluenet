/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshModelMulticast.h>
#include <mesh/cs_MeshModelMulticastAcked.h>
#include <mesh/cs_MeshModelMulticastNeighbours.h>
#include <mesh/cs_MeshModelUnicast.h>
#include <protocol/cs_Typedefs.h>

/**
 * Class that selects which model to use to send a message.
 */
class MeshModelSelector {
public:
	/**
	 * Init with pointer to all models.
	 * Could maybe be implemented neater if we have a base class for models.
	 */
	void init(
			MeshModelMulticast& multicastModel,
			MeshModelMulticastAcked& multicastAckedModel,
			MeshModelMulticastNeighbours& multicastNeighboursModel,
			MeshModelUnicast& unicastModel);

	/**
	 * Add item to the send queue of a suitable model.
	 */
	cs_ret_code_t addToQueue(MeshUtil::cs_mesh_queue_item_t& item);

	/**
	 * Remove an item from the send queue.
	 */
	cs_ret_code_t remFromQueue(MeshUtil::cs_mesh_queue_item_t & item);

private:
	MeshModelMulticast* _multicastModel                     = nullptr;
	MeshModelMulticastAcked* _multicastAckedModel           = nullptr;
	MeshModelMulticastNeighbours* _multicastNeighboursModel = nullptr;
	MeshModelUnicast* _unicastModel                         = nullptr;
};
