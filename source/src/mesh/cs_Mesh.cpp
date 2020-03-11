/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 12 Apr., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "mesh/cs_Mesh.h"
#include "mesh/cs_MeshCommon.h"

extern "C" {
#include "nrf_mesh.h"
#include "mesh_config.h"
#include "mesh_opt_core.h"
#include "mesh_stack.h"
#include "access.h"
#include "access_config.h"
#include "config_server.h"
#include "config_server_events.h"
#include "device_state_manager.h"
#include "mesh_provisionee.h"
#include "nrf_mesh_configure.h"
#include "nrf_mesh_events.h"
#include "net_state.h"
#include "scanner.h"
#include "uri.h"
#include "utils.h"

#include "log.h"
#include "access_internal.h"
#include "flash_manager_defrag.h"
#include "transport.h"
}

#include "cfg/cs_Boards.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_RNG.h"
#include "util/cs_BleError.h"
#include "util/cs_Utils.h"
#include "ble/cs_Stack.h"
#include "storage/cs_State.h"

#include <time/cs_SystemTime.h>
#include <protocol/cs_UartProtocol.h>



void Mesh::modelsInitCallback() {
	LOGMeshInfo("Initializing and adding models");
	_model.init();
	_model.setOwnAddress(_ownAddress);
}


Mesh::Mesh() {

}

cs_ret_code_t Mesh::init(const boards_config_t& board) {
	_msgHandler.init();
	return _core->init(board);
}

Mesh& Mesh::getInstance() {
	// Use static variant of singleton, no dynamic memory allocation
	static Mesh instance;
	return instance;
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
			[[maybe_unused]] const scanner_stats_t * stats = scanner_stats_get();
			LOGMeshDebug("success=%u crcFail=%u lenFail=%u memFail=%u", stats->successful_receives, stats->crc_failures, stats->length_out_of_bounds, stats->out_of_memory);
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
				_model.sendTime(&packet);
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
		_model.tick(tickCount);
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
	_model.sendMsg(&msg);

	return true;
}


