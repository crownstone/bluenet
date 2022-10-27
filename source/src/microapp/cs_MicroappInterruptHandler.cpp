/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 29, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <events/cs_Event.h>
#include <events/cs_EventDispatcher.h>
#include <microapp/cs_MicroappController.h>
#include <microapp/cs_MicroappInterruptHandler.h>
#include <microapp/cs_MicroappSdkUtil.h>

#define LogMicroappInterrupInfo LOGi
#define LogMicroappInterrupDebug LOGvv

/*
 * Enable throttling of BLE scanned devices by rssi.
 */
#define DEVELOPER_OPTION_THROTTLE_BY_RSSI 0

MicroappInterruptHandler::MicroappInterruptHandler() : EventListener() {
	EventDispatcher::getInstance().addListener(this);
}

void MicroappInterruptHandler::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_GPIO_UPDATE: {
			auto data = CS_TYPE_CAST(EVT_GPIO_UPDATE, event.data);
			onGpioUpdate(*data);
			break;
		}
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto data = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);
			onDeviceScanned(*data);
			break;
		}
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto data = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			onReceivedMeshMessage(*data);
			break;
		}

		// Central
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			auto data = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			onBleCentralConnectResult(*data);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED: {
			onBleCentralDisconnected();
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY: {
			auto data = CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY, event.data);
			onBleCentralDiscovery(*data);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT: {
			auto data = CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY_RESULT, event.data);
			onBleCentralDiscoveryResult(*data);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT: {
			auto data = CS_TYPE_CAST(EVT_BLE_CENTRAL_WRITE_RESULT, event.data);
			onBleCentralWriteResult(*data);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_READ_RESULT: {
			auto data = CS_TYPE_CAST(EVT_BLE_CENTRAL_READ_RESULT, event.data);
			onBleCentralReadResult(*data);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION: {
			auto data = CS_TYPE_CAST(EVT_BLE_CENTRAL_NOTIFICATION, event.data);
			onBleCentralNotification(*data);
			break;
		}

		// Peripheral
		case CS_TYPE::EVT_BLE_CONNECT: {
			auto data = CS_TYPE_CAST(EVT_BLE_CONNECT, event.data);
			onBlePeripheralConnect(*data);
			break;
		}
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			auto data = CS_TYPE_CAST(EVT_BLE_DISCONNECT, event.data);
			onBlePeripheralDisconnect(*data);
			break;
		}
		case CS_TYPE::EVT_ASSET_ACCEPTED: {
			auto data = CS_TYPE_CAST(EVT_ASSET_ACCEPTED, event.data);
			onAssetAccepted(*data);
			break;
		}
		default: {
			break;
		}
	}
	onBluenetEvent(event);
}

uint8_t* MicroappInterruptHandler::getOutputBuffer(MicroappSdkType type, uint8_t id) {
	if (!MicroappController::getInstance().allowSoftInterrupts(type, id)) {
		LogMicroappInterrupDebug("New interrupts blocked for type=%i id=%u, ignore event", type, id);
		return nullptr;
	}
	return MicroappController::getInstance().getOutputMicroappBuffer();
}

cs_ret_code_t MicroappInterruptHandler::onMessage(
		uint8_t appIndex, const cs_data_t& message, cs_data_t& resultBuffer, uint16_t& resultSize) {
	LogMicroappInterrupDebug("onMessage index=%u messageLen=%u", appIndex, message.len);
	if (message.len == 0) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	if (message.len > MICROAPP_SDK_MESSAGE_RECEIVED_MSG_MAX_SIZE) {
		LogMicroappInterrupInfo("Message too large: len=%u max=%u", message.len, MICROAPP_SDK_MESSAGE_RECEIVED_MSG_MAX_SIZE);
		return ERR_BUFFER_TOO_SMALL;
	}

	if (!MicroappController::getInstance().getRegisteredInterrupt(
				CS_MICROAPP_SDK_TYPE_MESSAGE, CS_MICROAPP_SDK_MSG_EVENT_RECEIVED_MSG)) {
		LogMicroappInterrupInfo("No interrupt registered");
		return ERR_NOT_STARTED;
	}
	if (!MicroappController::getInstance().allowSoftInterrupts(
				CS_MICROAPP_SDK_TYPE_MESSAGE, CS_MICROAPP_SDK_MSG_EVENT_RECEIVED_MSG)) {
		LogMicroappInterrupInfo("No interrupt allowed");
		return ERR_BUSY;
	}

	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_MESSAGE, CS_MICROAPP_SDK_MSG_EVENT_RECEIVED_MSG);
	if (outputBuffer == nullptr) {
		return ERR_UNSPECIFIED;
	}

	auto packet                  = reinterpret_cast<microapp_sdk_message_t*>(outputBuffer);
	packet->header.messageType   = CS_MICROAPP_SDK_TYPE_MESSAGE;
	packet->type                 = CS_MICROAPP_SDK_MSG_EVENT_RECEIVED_MSG;
	packet->receivedMessage.size = message.len;
	memcpy(packet->receivedMessage.data, message.data, packet->receivedMessage.size);

	MicroappController::getInstance().generateSoftInterrupt(
			CS_MICROAPP_SDK_TYPE_MESSAGE, CS_MICROAPP_SDK_MSG_EVENT_RECEIVED_MSG);
	return ERR_SUCCESS;
}

/*
 * Do some checks to validate if we want to generate an interrupt for the pin event
 * and if so, prepare the outgoing buffer and call generateSoftInterrupt()
 */
void MicroappInterruptHandler::onGpioUpdate(cs_gpio_update_t& event) {
	LogMicroappInterrupDebug("onGpioUpdate");
	uint8_t interruptPin  = MicroappSdkUtil::digitalPinToInterrupt(event.pinIndex);

	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_PIN, interruptPin);
	if (outputBuffer == nullptr) {
		LogMicroappInterrupDebug("No output buffer");
		return;
	}

	microapp_sdk_pin_t* pin = reinterpret_cast<microapp_sdk_pin_t*>(outputBuffer);
	pin->header.messageType = CS_MICROAPP_SDK_TYPE_PIN;
	pin->pin                = interruptPin;

	LogMicroappInterrupDebug("Incoming GPIO interrupt for microapp on virtual pin %i", interruptPin);
	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_PIN, interruptPin);
}

/*
 * Do some checks to validate if we want to generate an interrupt for the scanned device
 * and if so, prepare the outgoing buffer and call generateSoftInterrupt()
 */
void MicroappInterruptHandler::onDeviceScanned(scanned_device_t& dev) {
	if (!MicroappController::getInstance().microappData.isScanning) {
		return;
	}

	auto filter = MicroappController::getInstance().getScanFilter();
	switch (filter.type) {
		case CS_MICROAPP_SDK_BLE_SCAN_FILTER_RSSI: {
			if (dev.rssi < filter.rssi) {
				return;
			}
			break;
		}
		case CS_MICROAPP_SDK_BLE_SCAN_FILTER_MAC: {
			if (memcmp(filter.mac, dev.address, MAC_ADDRESS_LEN) != 0) {
				return;
			}
			break;
		}
		case CS_MICROAPP_SDK_BLE_SCAN_FILTER_NAME: {
			bool passedFilter = false;
			cs_data_t advData;
			cs_ret_code_t retCode;

			retCode = CsUtils::findAdvType(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, dev.data, dev.dataSize, &advData);
			if (retCode == ERR_SUCCESS && filter.name.size == advData.len) {
				passedFilter = (memcmp(filter.name.name, advData.data, advData.len) == 0);
			}

			retCode = CsUtils::findAdvType(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, dev.data, dev.dataSize, &advData);
			if (retCode == ERR_SUCCESS && filter.name.size == advData.len) {
				passedFilter = (memcmp(filter.name.name, advData.data, advData.len) == 0);
			}

			if (!passedFilter) {
				return;
			}
			break;
		}
		case CS_MICROAPP_SDK_BLE_SCAN_FILTER_SERVICE_16_BIT: {
			bool passedFilter = false;
			cs_data_t advData;
			cs_ret_code_t retCode;
			uint16_t* services;

			retCode = CsUtils::findAdvType(
					BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, dev.data, dev.dataSize, &advData);
			services = reinterpret_cast<uint16_t*>(advData.data);
			if (retCode == ERR_SUCCESS) {
				for (uint8_t i = 0; i < advData.len / sizeof(services[0]); ++i) {
					if (services[i] == filter.service16bit) {
						passedFilter = true;
						break;
					}
				}
			}

			retCode =
					CsUtils::findAdvType(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, dev.data, dev.dataSize, &advData);
			services = reinterpret_cast<uint16_t*>(advData.data);
			if (retCode == ERR_SUCCESS) {
				for (uint8_t i = 0; i < advData.len / sizeof(services[0]); ++i) {
					if (services[i] == filter.service16bit) {
						passedFilter = true;
						break;
					}
				}
			}

			if (!passedFilter) {
				return;
			}
			break;
		}
		default: {
			break;
		}
	}

	LogMicroappInterrupDebug("onDeviceScanned");

	// Write bluetooth device to buffer
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_SCAN);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	if (dev.dataSize > sizeof(ble->scan.eventScan.data)) {
		LOGw("BLE advertisement data too large");
		return;
	}

	ble->header.messageType          = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                        = CS_MICROAPP_SDK_BLE_SCAN;
	ble->scan.type                   = CS_MICROAPP_SDK_BLE_SCAN_EVENT_SCAN;
	ble->scan.eventScan.address.type = dev.addressType;
	memcpy(ble->scan.eventScan.address.address, dev.address, MAC_ADDRESS_LENGTH);
	ble->scan.eventScan.rssi = dev.rssi;
	ble->scan.eventScan.size = dev.dataSize;
	memcpy(ble->scan.eventScan.data, dev.data, dev.dataSize);

	LogMicroappInterrupDebug("Incoming BLE scanned device for microapp");
	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_SCAN);
}

/*
 * Do some checks to validate if we want to generate an interrupt for the received message
 * and if so, prepare the outgoing buffer and call generateSoftInterrupt()
 */
void MicroappInterruptHandler::onReceivedMeshMessage(MeshMsgEvent& event) {
	if (event.type != CS_MESH_MODEL_TYPE_MICROAPP) {
		// Mesh message received, but not for the microapp.
		return;
	}
	LogMicroappInterrupDebug("onReceivedMeshMessage");

	if (event.isReply) {
		// We received the empty reply.
		LogMicroappInterrupDebug("Reply received");
		return;
	}
	if (event.reply != nullptr) {
		// Send an empty reply.
		event.reply->type     = CS_MESH_MODEL_TYPE_MICROAPP;
		event.reply->dataSize = 0;
	}

	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_MESH, CS_MICROAPP_SDK_MESH_READ);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_mesh_t* meshMsg = reinterpret_cast<microapp_sdk_mesh_t*>(outputBuffer);
	meshMsg->header.messageType  = CS_MICROAPP_SDK_TYPE_MESH;
	meshMsg->type                = CS_MICROAPP_SDK_MESH_READ;
	meshMsg->stoneId             = event.srcStoneId;
	meshMsg->size                = event.msg.len;
	memcpy(meshMsg->data, event.msg.data, event.msg.len);

	LogMicroappInterrupDebug("Incoming mesh message for microapp");
	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_MESH, CS_MICROAPP_SDK_MESH_READ);
}

void MicroappInterruptHandler::onBleCentralConnectResult(cs_ret_code_t& retCode) {
	LogMicroappInterrupDebug("onBleCentralConnectResult");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble          = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType          = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                        = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type                = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_CONNECT;
	ble->central.connectionHandle    = Stack::getInstance().getConnectionHandle();
	ble->central.eventConnect.result = MicroappSdkUtil::bluenetResultToMicroapp(retCode);

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBleCentralDisconnected() {
	LogMicroappInterrupDebug("onBleCentralDisconnected");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble       = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType       = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                     = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type             = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_DISCONNECT;
	ble->central.connectionHandle = Stack::getInstance().getConnectionHandle();

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBleCentralDiscovery(ble_central_discovery_t& event) {
	LogMicroappInterrupDebug("onBleCentralDiscovery");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble                            = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType                            = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                                          = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type                                  = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_DISCOVER;
	ble->central.connectionHandle                      = Stack::getInstance().getConnectionHandle();
	ble->central.eventDiscover.serviceUuid             = MicroappSdkUtil::convertUuid(event.serviceUuid);
	ble->central.eventDiscover.uuid                    = MicroappSdkUtil::convertUuid(event.uuid);
	ble->central.eventDiscover.valueHandle             = event.valueHandle;
	ble->central.eventDiscover.cccdHandle              = event.cccdHandle;
	ble->central.eventDiscover.options.read            = event.flags.read;
	ble->central.eventDiscover.options.writeNoResponse = event.flags.write_no_ack;
	ble->central.eventDiscover.options.write           = event.flags.write_with_ack;
	ble->central.eventDiscover.options.notify          = event.flags.notify;
	ble->central.eventDiscover.options.indicate        = event.flags.indicate;

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBleCentralDiscoveryResult(cs_ret_code_t& retCode) {
	LogMicroappInterrupDebug("onBleCentralDiscoveryResult");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble               = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType               = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                             = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type                     = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_DISCOVER_DONE;
	ble->central.connectionHandle         = Stack::getInstance().getConnectionHandle();
	ble->central.eventDiscoverDone.result = MicroappSdkUtil::bluenetResultToMicroapp(retCode);

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBleCentralWriteResult(ble_central_write_result_t& event) {
	LogMicroappInterrupDebug("onBleCentralWriteResult");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble        = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType        = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                      = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type              = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_WRITE;
	ble->central.connectionHandle  = Stack::getInstance().getConnectionHandle();  // TODO: get handle from event.
	ble->central.eventWrite.handle = event.handle;
	ble->central.eventWrite.result = MicroappSdkUtil::bluenetResultToMicroapp(event.retCode);

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBleCentralReadResult(ble_central_read_result_t& event) {
	LogMicroappInterrupDebug("onBleCentralReadResult");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble            = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType            = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                          = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type                  = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_READ;
	ble->central.connectionHandle      = Stack::getInstance().getConnectionHandle();  // TODO: get handle from event.
	ble->central.eventRead.valueHandle = event.handle;
	ble->central.eventRead.result      = MicroappSdkUtil::bluenetResultToMicroapp(event.retCode);

	ble->central.eventRead.size        = std::min(event.data.len, MICROAPP_SDK_BLE_CENTRAL_EVENT_READ_DATA_MAX_SIZE);
	memcpy(ble->central.eventRead.data, event.data.data, ble->central.eventRead.size);

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBleCentralNotification(ble_central_notification_t& event) {
	LogMicroappInterrupDebug("onBleCentralNotification");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble       = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType       = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                     = CS_MICROAPP_SDK_BLE_CENTRAL;
	ble->central.type             = CS_MICROAPP_SDK_BLE_CENTRAL_EVENT_NOTIFICATION;
	ble->central.connectionHandle = Stack::getInstance().getConnectionHandle();  // TODO: get handle from event.
	ble->central.eventNotification.valueHandle = event.handle;

	ble->central.eventNotification.size =
			std::min(event.data.len, MICROAPP_SDK_BLE_CENTRAL_EVENT_NOTIFICATION_DATA_MAX_SIZE);
	memcpy(ble->central.eventNotification.data, event.data.data, ble->central.eventNotification.size);

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);
}

void MicroappInterruptHandler::onBlePeripheralConnect(ble_connected_t& event) {
	LogMicroappInterrupDebug("onBlePeripheralConnect");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble                   = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType                   = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                                 = CS_MICROAPP_SDK_BLE_PERIPHERAL;
	ble->peripheral.type                      = CS_MICROAPP_SDK_BLE_PERIPHERAL_EVENT_CONNECT;
	ble->peripheral.connectionHandle          = event.connectionHandle;

	ble->peripheral.eventConnect.address.type = event.address.addressType;
	memcpy(ble->peripheral.eventConnect.address.address,
		   event.address.address,
		   sizeof(ble->peripheral.eventConnect.address.address));

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
}

void MicroappInterruptHandler::onBlePeripheralDisconnect(uint16_t connectionHandle) {
	LogMicroappInterrupDebug("onBlePeripheralConnect");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble          = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType          = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                        = CS_MICROAPP_SDK_BLE_PERIPHERAL;
	ble->peripheral.type             = CS_MICROAPP_SDK_BLE_PERIPHERAL_EVENT_DISCONNECT;
	ble->peripheral.connectionHandle = connectionHandle;

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
}

void MicroappInterruptHandler::onBlePeripheralWrite(
		uint16_t connectionHandle, uint16_t characteristicHandle, cs_data_t value) {
	LogMicroappInterrupDebug("onBlePeripheralWrite");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble          = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType          = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                        = CS_MICROAPP_SDK_BLE_PERIPHERAL;
	ble->peripheral.type             = CS_MICROAPP_SDK_BLE_PERIPHERAL_EVENT_WRITE;
	ble->peripheral.connectionHandle = connectionHandle;
	ble->peripheral.handle           = characteristicHandle;
	ble->peripheral.eventWrite.size  = value.len;

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
}

void MicroappInterruptHandler::onBlePeripheralSubscription(
		uint16_t connectionHandle, uint16_t characteristicHandle, bool subscribed) {
	LogMicroappInterrupDebug("onBlePeripheralSubscription subscribed=%u", subscribed);
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type               = CS_MICROAPP_SDK_BLE_PERIPHERAL;
	if (subscribed) {
		ble->peripheral.type = CS_MICROAPP_SDK_BLE_PERIPHERAL_EVENT_SUBSCRIBE;
	}
	else {
		ble->peripheral.type = CS_MICROAPP_SDK_BLE_PERIPHERAL_EVENT_UNSUBSCRIBE;
	}
	ble->peripheral.connectionHandle = connectionHandle;
	ble->peripheral.handle           = characteristicHandle;

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
}

void MicroappInterruptHandler::onBlePeripheralNotififyDone(uint16_t connectionHandle, uint16_t characteristicHandle) {
	// Right now, this doesn't filter out events for the crownstone service.
	LogMicroappInterrupDebug("onBlePeripheralNotififyDone");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
	if (outputBuffer == nullptr) {
		return;
	}

	microapp_sdk_ble_t* ble          = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);
	ble->header.messageType          = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type                        = CS_MICROAPP_SDK_BLE_PERIPHERAL;
	ble->peripheral.type             = CS_MICROAPP_SDK_BLE_PERIPHERAL_EVENT_NOTIFICATION_DONE;
	ble->peripheral.connectionHandle = connectionHandle;
	ble->peripheral.handle           = characteristicHandle;

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);
}

void MicroappInterruptHandler::onAssetAccepted(AssetAcceptedEvent& event) {
	LogMicroappInterrupDebug("onAssetAccepted");
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_ASSETS, CS_MICROAPP_SDK_ASSET_EVENT);
	if (outputBuffer == nullptr) {
		return;
	}
	asset_id_t assetId = event._primaryFilter.getAssetId(event._asset);
	microapp_sdk_asset_t* asset = reinterpret_cast<microapp_sdk_asset_t*>(outputBuffer);
	asset->header.messageType = CS_MICROAPP_SDK_TYPE_ASSETS;
	asset->type               = CS_MICROAPP_SDK_ASSET_EVENT;
	memcpy(asset->event.assetId, assetId.data, sizeof(asset_id_t));
	asset->event.rssi         = event._asset.rssi;
	asset->event.channel      = event._asset.channel;
	asset->event.profileId    = *(event._primaryFilter.filterdata().metadata().profileId());
	asset->event.filterId     = event._filterIndex;
	asset->event.address.type = event._asset.addressType;
	memcpy(asset->event.address.address, event._asset.address, MAC_ADDRESS_LEN);

	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_ASSETS, CS_MICROAPP_SDK_ASSET_EVENT);
}

void MicroappInterruptHandler::onBluenetEvent(event_t& event) {
	if (!MicroappController::getInstance().isEventInterruptRegistered(event.type)) {
		return;
	}

	LogMicroappInterrupDebug("onBluenetEvent type=%u", event.type);
	uint8_t* outputBuffer = getOutputBuffer(CS_MICROAPP_SDK_TYPE_BLUENET_EVENT, 0);
	if (outputBuffer == nullptr) {
		return;
	}

	auto packet                = reinterpret_cast<microapp_sdk_bluenet_event_t*>(outputBuffer);
	packet->header.messageType = CS_MICROAPP_SDK_TYPE_BLUENET_EVENT;
	packet->type               = CS_MICROAPP_SDK_BLUENET_EVENT_EVENT;
	packet->eventType          = static_cast<uint16_t>(event.type);
	packet->event.size         = std::min(event.size, MICROAPP_SDK_BLUENET_EVENT_EVENT_MAX_SIZE);
	memcpy(packet->event.data, event.data, packet->event.size);
	MicroappController::getInstance().generateSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLUENET_EVENT, 0);
}
