/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 29, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_Event.h>
#include <events/cs_EventListener.h>
#include <mesh/cs_MeshMsgEvent.h>
#include <structs/cs_PacketsInternal.h>

#define LogMicroappRequestHandlerDebug LOGvv

/**
 * The class MicroappRequestHandler has functionality to store a second app (and perhaps in the future even more apps)
 * on another part of the flash memory.
 */
class MicroappInterruptHandler : public EventListener {
public:
	static MicroappInterruptHandler& getInstance() {
		static MicroappInterruptHandler instance;
		return instance;
	}

private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappInterruptHandler();
	MicroappInterruptHandler(MicroappInterruptHandler const&);
	void operator=(MicroappInterruptHandler const&);

	/**
	 * Check whether an interrupt is allowed to be sent and return the output buffer.
	 *
	 * @return nullptr       When no interrupt can be sent.
	 * @return buffer        The buffer to fill for the interrupt.
	 */
	uint8_t* getOutputBuffer(MicroappSdkMessageType type, uint8_t id);

	/**
	 * Handle a GPIO event
	 */
	void onGpioUpdate(cs_gpio_update_t& event);

	/**
	 * Handle a scanned BLE device.
	 */
	void onDeviceScanned(scanned_device_t& dev);

	/**
	 * Handle a received mesh message and determine whether to forward it to the microapp.
	 *
	 * @param event the EVT_RECV_MESH_MSG event data
	 */
	void onReceivedMeshMessage(MeshMsgEvent& event);

	void onBleCentralConnectResult(cs_ret_code_t& retCode);

	void onBleCentralDisconnected();

	void onBleCentralDiscovery(ble_central_discovery_t& event);
	void onBleCentralDiscoveryResult(cs_ret_code_t& retCode);
	void onBleCentralWriteResult(cs_ret_code_t& retCode);
	void onBleCentralReadResult(ble_central_read_result_t& event);
	void onBleCentralNotification(ble_central_notification_t& event);
	void onBlePeripheralConnect(ble_connected_t& event);
	void onBlePeripheralDisconnect(uint16_t connectionHandle);

public:
	void onBlePeripheralWrite(uint16_t connectionHandle, uint16_t characteristicHandle, cs_data_t value);
	void onBlePeripheralSubscription(uint16_t connectionHandle, uint16_t characteristicHandle, bool subscribed);
	void onBlePeripheralNotififyDone(uint16_t connectionHandle, uint16_t characteristicHandle);

	/**
	 * Handle events
	 */
	void handleEvent(event_t& event);
};
