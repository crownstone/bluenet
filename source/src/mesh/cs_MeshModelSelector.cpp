/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshModelSelector.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <util/cs_BleError.h>

void MeshModelSelector::init(MeshModelMulticast* multicastModel, MeshModelUnicast* unicastModel) {
	_multicastModel = multicastModel;
	_unicastModel = unicastModel;
}

cs_ret_code_t MeshModelSelector::addToQueue(MeshUtil::cs_mesh_queue_item_t& item) {
	LOGd("addToQueue");
	assert(_multicastModel != nullptr && _unicastModel != nullptr, "Model not set");
	if (item.metaData.targetId == 0) {
		return _multicastModel->addToQueue(item);
	}
	else {
		return _unicastModel->addToQueue(item);
	}
}

cs_ret_code_t MeshModelSelector::remFromQueue(MeshUtil::cs_mesh_queue_item_meta_data_t item) {
	LOGd("remFromQueue");
	assert(_multicastModel != nullptr && _unicastModel != nullptr, "Model not set");
	if (item.targetId == 0) {
		return _multicastModel->remFromQueue((cs_mesh_model_msg_type_t)item.type, item.id);
	}
	else {
		return _unicastModel->remFromQueue((cs_mesh_model_msg_type_t)item.type, item.id);
	}
}
