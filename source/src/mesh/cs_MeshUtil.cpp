/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Serial.h>
#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshUtil.h>

namespace MeshUtil {

int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData) {
	switch (metaData->source) {
	case NRF_MESH_RX_SOURCE_SCANNER:
		return metaData->params.scanner.rssi;
	case NRF_MESH_RX_SOURCE_GATT:
		// TODO: return connection rssi?
		return -10;
//	case NRF_MESH_RX_SOURCE_FRIEND:
//		// TODO: is this correct?
//		return metaData->params.scanner.rssi;
//		break;
//	case NRF_MESH_RX_SOURCE_LOW_POWER:
//		// TODO: is this correct?
//		return metaData->params.scanner.rssi;
//		break;
	case NRF_MESH_RX_SOURCE_INSTABURST:
		return metaData->params.instaburst.rssi;
		break;
	case NRF_MESH_RX_SOURCE_LOOPBACK:
		return -1;
		break;
	}
	return 0;
}

void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr) {
	switch (addr->type) {
	case NRF_MESH_ADDRESS_TYPE_INVALID:
		LOGMeshModelVerbose("%s type=invalid", prefix);
		break;
	case NRF_MESH_ADDRESS_TYPE_UNICAST:
		LOGMeshModelVerbose("%s type=unicast id=%u", prefix, addr->value);
		break;
	case NRF_MESH_ADDRESS_TYPE_VIRTUAL:{
		//128-bit virtual label UUID,
		__attribute__((unused)) uint32_t* uuid1 = (uint32_t*)(addr->p_virtual_uuid);
		__attribute__((unused)) uint32_t* uuid2 = (uint32_t*)(addr->p_virtual_uuid + 4);
		__attribute__((unused)) uint32_t* uuid3 = (uint32_t*)(addr->p_virtual_uuid + 8);
		__attribute__((unused)) uint32_t* uuid4 = (uint32_t*)(addr->p_virtual_uuid + 12);
		LOGMeshModelVerbose("%s type=virtual id=%u uuid=%x%x%x%x", prefix, addr->value, uuid1, uuid2, uuid3, uuid4);
		break;
	}
	case NRF_MESH_ADDRESS_TYPE_GROUP:
		LOGMeshModelVerbose("%s type=group id=%u", prefix, addr->value);
		break;
	}
}

}
