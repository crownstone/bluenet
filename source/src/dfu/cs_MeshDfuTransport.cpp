/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_MeshDfuTransport.h>
#include <dfu/cs_MeshDfuConstants.h>

cs_ret_code_t MeshDfuTransport::init() {
	if (!_firstInit) {
		return ERR_SUCCESS;
	}

	// Make use of the fact ERR_SUCCESS = 0, to avoid many if statements.
	cs_ret_code_t retCode = ERR_SUCCESS;
	retCode |= _controlPointUuid.fromFullUuid(MeshDfuConstants::DFUAdapter::controlPointString);
	retCode |= _dataPointUuid.fromFullUuid(MeshDfuConstants::DFUAdapter::dataPointString);

	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	listen();

	_firstInit = false;
	return ERR_SUCCESS;
}
