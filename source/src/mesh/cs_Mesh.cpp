/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 12 Apr., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <cfg/cs_Boards.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_RNG.h>
#include <mesh/cs_Mesh.h>
#include <mesh/cs_MeshCommon.h>
//#include <protocol/cs_UartProtocol.h>
//#include <storage/cs_State.h>
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
	_msgHandler.init();
	_core->registerModelInitCallback([&]() -> void {
		modelsInitCallback();
	});
	_core->registerScanCallback([&](const nrf_mesh_adv_packet_rx_data_t *scanData) -> void {
		_scanner.onScan(scanData);
	});
	return _core->init(board);
}

void Mesh::modelsInitCallback() {
	LOGMeshInfo("Initializing and adding models");
	_modelMulticast.init(0);
	_modelUnicast.init(1);
}

void Mesh::start() {
	_core->start();
	this->listen();
}

void Mesh::stop() {
	assert(false, "TODO");
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

void Mesh::advertise(IBeacon* ibeacon) {
	LOGd("advertise ibeacon major=%u minor=%u", ibeacon->getMajor(), ibeacon->getMinor());
	_advertiser.advertise(ibeacon);
}

void Mesh::handleEvent(event_t & event) {
	switch (event.type) {
	case CS_TYPE::EVT_TICK: {
		TYPIFY(EVT_TICK) tickCount = *((TYPIFY(EVT_TICK)*)event.data);
		if (tickCount % (500/TICK_INTERVAL_MS) == 0) {
			if (Stack::getInstance().isScanning()) {
//				Stack::getInstance().stopScanning();
			}
			else {
//				Stack::getInstance().startScanning();
			}
		}
		if (_sendStateTimeCountdown-- == 0) {
			uint8_t rand8;
			RNG::fillBuffer(&rand8, 1);
			uint32_t randMs = MESH_SEND_TIME_INTERVAL_MS + rand8 * MESH_SEND_TIME_INTERVAL_MS_VARIATION / 255;
			_sendStateTimeCountdown = randMs / TICK_INTERVAL_MS;

			Time time = SystemTime::posix();
			if (time.isValid()) {
				cs_mesh_model_msg_time_t packet;
				packet.timestamp = time.timestamp();
				_msgSender.sendTime(&packet);
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

					if(!_synced){
						LOGw("Sync failed");
						event_t syncFailEvent(CS_TYPE::EVT_MESH_SYNC_FAILED);
						syncFailEvent.dispatch();

						// yes, we know that sync failed, we're just misusing the _synced variable.
						// (setting it to true will prevent any further sync requests.)
						_synced = true;
					}
				}
			}
		}
#if MESH_MODEL_TEST_MSG == 2
		if (_ownAddress == 2 && tickCount % (1000 / TICK_INTERVAL_MS) == 0) {
			sendTestMsg();
		}
#endif
		_modelMulticast.tick(tickCount);
		_modelUnicast.tick(tickCount);
		break;
	}
	case CS_TYPE::CMD_ENABLE_MESH: {
#if BUILD_MESHING == 1
			uint8_t enable = *(uint8_t*)event.data;
			if (enable) {
				start();
			}
			else {
				stop();
			}
			UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_MESH_ENABLED, &enable, 1);
#endif
			break;
	}
	case CS_TYPE::EVT_GENERIC_TEST: {
		LOGd("generic test event received, calling requestSync()");
		requestSync();
		break;
	}
	default:
		break;
	}
}

void Mesh::startSync() {
	_synced = !requestSync();
	_syncCountdown = MESH_SYNC_RETRY_INTERVAL_MS / TICK_INTERVAL_MS;
	_syncFailedCountdown = MESH_SYNC_GIVE_UP_MS / TICK_INTERVAL_MS;
}

bool Mesh::requestSync(bool propagateSyncMessageOverMesh) {
//	while (SystemTime::up() < 5) {
//		// LOGMeshInfo("Mesh::requestSync: nope");
//		return false;
//	}

	// Retrieve which data should be requested from event handlers.
	TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING) syncRequest;
	syncRequest.bitmask = 0;
	event_t event(CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING, &syncRequest, sizeof(syncRequest));
	event.dispatch();

	LOGMeshInfo("requestSync bitmask=%u", syncRequest.bitmask);

	if (syncRequest.bitmask == 0 || !propagateSyncMessageOverMesh){
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


