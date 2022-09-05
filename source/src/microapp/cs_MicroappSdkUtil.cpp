/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 29, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_UUID.h>
#include <cs_MicroappStructs.h>
#include <microapp/cs_MicroappSdkUtil.h>
#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_Typedefs.h>

uint8_t MicroappSdkUtil::digitalPinToInterrupt(uint8_t pinIndex) {
	return pinIndex;
}

uint8_t MicroappSdkUtil::interruptToDigitalPin(uint8_t interrupt) {
	return interrupt;
}

MicroappSdkAck MicroappSdkUtil::bluenetResultToMicroapp(cs_ret_code_t retCode) {
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE: {
			return CS_MICROAPP_SDK_ACK_SUCCESS;
		}
		case ERR_WAIT_FOR_SUCCESS: {
			return CS_MICROAPP_SDK_ACK_IN_PROGRESS;
		}
		case ERR_NOT_FOUND: {
			return CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
		}
		case ERR_NO_SPACE: {
			return CS_MICROAPP_SDK_ACK_ERR_NO_SPACE;
		}
		case ERR_NOT_IMPLEMENTED: {
			return CS_MICROAPP_SDK_ACK_ERR_NOT_IMPLEMENTED;
		}
		case ERR_BUSY: {
			return CS_MICROAPP_SDK_ACK_ERR_BUSY;
		}
		case ERR_NOT_AVAILABLE: {
			return CS_MICROAPP_SDK_ACK_ERR_DISABLED;
		}
		default: {
			return CS_MICROAPP_SDK_ACK_ERROR;
		}
	}
}

microapp_sdk_ble_uuid_t MicroappSdkUtil::convertUuid(const UUID& uuid) {
	microapp_sdk_ble_uuid_t convertedUuid = {
			.type = uuid.getUuid().type,
			.uuid = uuid.getUuid().uuid,
	};
	return convertedUuid;
}

UUID MicroappSdkUtil::convertUuid(const microapp_sdk_ble_uuid_t& uuid) {
	ble_uuid_t bleUuid = {
			.uuid = uuid.uuid,
			.type = uuid.type,
	};
	return UUID(bleUuid);
}
