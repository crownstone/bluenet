/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 12 Apr., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <cfg/cs_Boards.h>
#include <logging/cs_Logger.h>
#include <drivers/cs_RNG.h>
#include <mesh/cs_Mesh.h>
#include <mesh/cs_MeshCommon.h>
#include <uart/cs_UartHandler.h>
#include <storage/cs_State.h>
#include <third/std/function.h>
#include <time/cs_SystemTime.h>
//#include <util/cs_BleError.h>
//#include <util/cs_Utils.h>

Mesh::Mesh() {
	_core = &(MeshCore::getInstance());
}

Mesh& Mesh::getInstance() {
	// Use static variant of singleton, no dynamic memory allocation
	static Mesh instance;
	return instance;
}

bool Mesh::checkFlashValid() {
	bool valid = _core->isFlashValid();
	if (!valid) {
		if (_core->eraseAllPages() != ERR_SUCCESS) {
			// Only option left is to reboot and see if things work out next time.
			APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE);
		}
	}
	return valid;
}

cs_ret_code_t Mesh::init(const boards_config_t& board) {
	LOGi("init");
	_msgHandler.init();
	_core->registerModelInitCallback([&]() -> void {
		initModels();
	});
	_core->registerModelConfigureCallback([&](dsm_handle_t appkeyHandle) -> void {
		configureModels(appkeyHandle);
	});
	_core->registerScanCallback([&](const nrf_mesh_adv_packet_rx_data_t *scanData) -> void {
		_scanner.onScan(scanData);
	});
	_modelSelector.init(_modelMulticast, _modelMulticastAcked, _modelMulticastNeighbours, _modelUnicast);
	_msgSender.init(&_modelSelector);

	cs_ret_code_t retCode = _core->init(board);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	_msgSender.listen();
	this->listen();
	return retCode;
}

void Mesh::initModels() {
	LOGi("Initializing and adding models");
	_modelMulticast.registerMsgHandler([&](const MeshUtil::cs_mesh_received_msg_t& msg) -> void {
		_msgHandler.handleMsg(msg, nullptr);
	});
	_modelMulticast.init(0);

	_modelMulticastAcked.registerMsgHandler([&](const MeshUtil::cs_mesh_received_msg_t& msg, mesh_reply_t* reply) -> void {
		_msgHandler.handleMsg(msg, reply);
	});
	_modelMulticastAcked.init(1);

	_modelUnicast.registerMsgHandler([&](const MeshUtil::cs_mesh_received_msg_t& msg, mesh_reply_t* reply) -> void {
		_msgHandler.handleMsg(msg, reply);
	});
	_modelUnicast.init(2);

	_modelMulticastNeighbours.registerMsgHandler([&](const MeshUtil::cs_mesh_received_msg_t& msg) -> void {
			_msgHandler.handleMsg(msg, nullptr);
	});
	_modelMulticastNeighbours.init(3);
}

void Mesh::configureModels(dsm_handle_t appkeyHandle) {
	_modelMulticast.configureSelf(appkeyHandle);
	_modelMulticastAcked.configureSelf(appkeyHandle);
	_modelUnicast.configureSelf(appkeyHandle);
	_modelMulticastNeighbours.configureSelf(appkeyHandle);
}

void Mesh::start() {
	if (_enabled) {
		LOGi("start");
		_core->start();
	}
}

void Mesh::stop() {
	LOGi("stop");
	_core->stop();
}

void Mesh::initAdvertiser() {
	_advertiser.init();

	ble_gap_addr_t address;
	uint32_t retCode = sd_ble_gap_addr_get(&address);
	APP_ERROR_CHECK(retCode);
	// have non-connectable address one value higher than connectable one
	address.addr[0] += 1;
	_advertiser.setMacAddress(address.addr);

	TYPIFY(CONFIG_ADV_INTERVAL) advInterval;
	State::getInstance().get(CS_TYPE::CONFIG_ADV_INTERVAL, &advInterval, sizeof(advInterval));
	uint32_t advIntervalMs = advInterval * 625 / 1000;
	_advertiser.setInterval(advIntervalMs);

	TYPIFY(CONFIG_TX_POWER) txPower;
	State::getInstance().get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	_advertiser.setTxPower(txPower);

	_advertiser.start();
}

void Mesh::advertiseIbeacon() {
	_advertiser.advertiseIbeacon(0);
}

void Mesh::stopAdvertising() {
	_advertiser.stop();
}

void Mesh::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_TICK: {
			TYPIFY(EVT_TICK) tickCount = *((TYPIFY(EVT_TICK)*)event.data);
			onTick(tickCount);
			break;
		}
		case CS_TYPE::CMD_ENABLE_MESH: {
#if BUILD_MESHING == 1
			_enabled = *(TYPIFY(CMD_ENABLE_MESH)*)event.data;
			if (_enabled) {
				start();
			}
			else {
				stop();
			}
			UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_ENABLED, &_enabled, 1);
#endif
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_START: {
			// An outgoing connection is made, stop listening to mesh,
			// so that the softdevice has time to listen for advertisements.
			stop();
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			// We can start listening to the mesh again, regardless of the result.
			start();
			break;
		}

		default:
			break;
	}
}

void Mesh::onTick(uint32_t tickCount) {
	if (tickCount % (500/TICK_INTERVAL_MS) == 0) {
		if (Stack::getInstance().isScanning()) {
//				Stack::getInstance().stopScanning();
		}
		else {
//				Stack::getInstance().startScanning();
		}
	}

	if (!_synced) {
		if (_syncCountdown) {
			--_syncCountdown;
		}
		else {
			_synced = !requestSync();
			_syncCountdown = MESH_SYNC_RETRY_INTERVAL_MS / TICK_INTERVAL_MS;
		}
		if (_syncFailedCountdown) {
			if (--_syncFailedCountdown == 0) {
				// Do one last check, internally to see if the previous requestSync succeeded.
				// but don't send anything over the mesh. Our chance has passed.
				_synced = !requestSync(false);

				if (!_synced) {
					LOGi("Sync failed");
					event_t syncFailEvent(CS_TYPE::EVT_MESH_SYNC_FAILED);
					syncFailEvent.dispatch();

					// yes, we know that sync failed, we're just misusing the _synced variable.
					// (setting it to true will prevent any further sync requests.)
					_synced = true;
				}
			}
		}
	}
#if MESH_MODEL_TEST_MSG == 1
	if (_core->getUnicastAddress() == 2 && tickCount % (100 / TICK_INTERVAL_MS) == 0) {
		_msgSender.sendTestMsg();
	}
#elif MESH_MODEL_TEST_MSG == 2
	if (_core->getUnicastAddress() == 2 && tickCount % (1000 / TICK_INTERVAL_MS) == 0) {
		_msgSender.sendTestMsg();
	}
#endif
	_modelMulticast.tick(tickCount);
	_modelMulticastAcked.tick(tickCount);
	_modelMulticastNeighbours.tick(tickCount);
	_modelUnicast.tick(tickCount);
}

void Mesh::startSync() {
	_synced = !requestSync();
	_syncCountdown = MESH_SYNC_RETRY_INTERVAL_MS / TICK_INTERVAL_MS;
	_syncFailedCountdown = MESH_SYNC_GIVE_UP_MS / TICK_INTERVAL_MS;
}

bool Mesh::requestSync(bool propagateSyncMessageOverMesh) {
	// Retrieve which data should be requested from event handlers.
	TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING) syncRequest;
	syncRequest.bitmask = 0;
	event_t event(CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING, &syncRequest, sizeof(syncRequest));
	event.dispatch();

	LOGd("requestSync bitmask=%u", syncRequest.bitmask);

	if (syncRequest.bitmask == 0 || !propagateSyncMessageOverMesh) {
		// bitmask == 0 implies sync is unnecessary 
		// propagateSyncMessageOverMesh==false implies we return here, before contacting the mesh.
		return syncRequest.bitmask != 0;
	}

	// Make sure that event data type is equal to mesh msg type.
	cs_mesh_model_msg_sync_request_t* requestMsg = &syncRequest;

	// Set crownstone id.
	TYPIFY(CONFIG_CROWNSTONE_ID) id;
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &id, sizeof(id));
	requestMsg->id = id;

	// Send the message over the mesh.
	cs_mesh_msg_t msg = {};
	msg.payload = reinterpret_cast<uint8_t*>(requestMsg);
	msg.size = sizeof(*requestMsg);
	msg.type = CS_MESH_MODEL_TYPE_SYNC_REQUEST;
	msg.reliability = CS_MESH_RELIABILITY_MEDIUM;
	_msgSender.sendMsg(&msg);

	return true;
}


