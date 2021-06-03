/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshModelSelector.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <util/cs_BleError.h>

void MeshModelSelector::init(
		MeshModelMulticast& multicastModel,
		MeshModelMulticastAcked& multicastAckedModel,
		MeshModelMulticastNeighbours& multicastNeighboursModel,
		MeshModelUnicast& unicastModel) {
	_multicastModel           = &multicastModel;
	_multicastAckedModel      = &multicastAckedModel;
	_multicastNeighboursModel = &multicastNeighboursModel;
	_unicastModel             = &unicastModel;
}

cs_ret_code_t MeshModelSelector::addToQueue(MeshUtil::cs_mesh_queue_item_t& item) {
	assert(_multicastModel != nullptr && _unicastModel != nullptr, "Model not set");
	if (item.broadcast) {
		if (item.reliable) {
			if (item.noHop) {
				return ERR_NOT_IMPLEMENTED;
			}
			else {
				return _multicastAckedModel->addToQueue(item);
			}
		}
		else {
			if (item.noHop) {
				return _multicastNeighboursModel->addToQueue(item);
			}
			else {
				return _multicastModel->addToQueue(item);
			}
		}
	}
	else {
		if (item.reliable) {
			if (item.numIds == 1) {
				// Unicast model can send with and without hops.
				return _unicastModel->addToQueue(item);
			}
			else {
				return ERR_NOT_IMPLEMENTED;
			}
		}
		else {
			return ERR_NOT_IMPLEMENTED;
		}
	}
}

cs_ret_code_t MeshModelSelector::remFromQueue(MeshUtil::cs_mesh_queue_item_t & item) {
	assert(_multicastModel != nullptr && _unicastModel != nullptr, "Model not set");
	if (item.broadcast) {
		if (item.reliable) {
			return _multicastAckedModel->remFromQueue((cs_mesh_model_msg_type_t)item.metaData.type, item.metaData.id);
		}
		else {
			return _multicastModel->remFromQueue((cs_mesh_model_msg_type_t)item.metaData.type, item.metaData.id);
		}
	}
	else {
		if (item.reliable) {
			return _unicastModel->remFromQueue((cs_mesh_model_msg_type_t)item.metaData.type, item.metaData.id);
		}
		else {
			return ERR_NOT_IMPLEMENTED;
		}
	}
}
