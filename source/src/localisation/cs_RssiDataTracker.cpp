#include <localisation/cs_RssiDataTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <events/cs_Event.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <test/cs_Test.h>

#include <cmath>

#define RSSIDATATRACKER_LOGd LOGd
#define RSSIDATATRACKER_LOGv LOGd

// ------------ RssiDataTracker methods ------------

RssiDataTracker::RssiDataTracker() :
		flushRoutine([this]() {
			return flushAggregatedRssiData();
		}) {
}

void RssiDataTracker::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &my_id,
	        sizeof(my_id));
	RSSIDATATRACKER_LOGd("RssiDataTracker: my_id %d",my_id);
}

// ------------ Recording ping stuff ------------


void RssiDataTracker::recordRssiValue(stone_id_t sender_id, int8_t rssi, uint8_t channel) {
	auto channel_index = channel - CHANNEL_START;

	if (channel_index < 0 || CHANNEL_COUNT <= channel_index ){
		return;
	}

	auto& recorder = recorder_map[channel_index ][sender_id];

	// If we aqcuired a lot of data, need to reduce to prevent overflow.
	if (recorder.isNumericPrecisionLow()) {
		recorder.reduceCount();
	}

	// log the data
	recorder.addValue(rssi);
	last_rssi_map[channel_index ][sender_id] = rssi;
}

// ------------ Sending Rssi Data ------------

uint8_t RssiDataTracker::getVarianceDescriptor(float variance) {
	variance = std::abs(variance);
	if(variance <  2) return 0;
	if(variance <  4) return 1;
	if(variance <  8) return 2;
	if(variance < 12) return 3;
	if(variance < 16) return 4;
	if(variance < 24) return 5;
	if(variance < 32) return 6;
	return 7;
}


uint32_t RssiDataTracker::flushAggregatedRssiData() {
	RSSIDATATRACKER_LOGd("flushAggregatedRssiData");
	// start flushing phase, here we wait quite a bit shorter until the map is empty.

	// ** begin burst loop **
	for (auto main_iter = recorder_map[0].begin();
			main_iter != recorder_map[0].end();
			++main_iter) {
		stone_id_t id = main_iter->first;

		RSSIDATATRACKER_LOGd("Burst start for id(%d)", id);

		// the maps may not have the same key sets, this depends
		// on possible loss differences between the channels.

		// Intersecting the key sets of these maps is pure ugly.
		// Fortunately we only doing this once every half hour or so.
		decltype(main_iter) rec_iters[] = {
				main_iter,
				recorder_map[1].find(id),
				recorder_map[2].find(id),
		};

		bool all_maps_have_sufficient_data_for_id = true;
		for(auto i = 0; i < CHANNEL_COUNT; ++i) {
			if (rec_iters[i] == recorder_map[i].end() ||
					rec_iters[i]->second.getCount() < Settings.min_samples_to_trigger_burst) {
				all_maps_have_sufficient_data_for_id = false;
				break;
			}
		}

		if (all_maps_have_sufficient_data_for_id) {
			rssi_data_message_t rssi_data;

			rssi_data.sample_count_ch37 = rec_iters[0]->second.getCount();
			rssi_data.sample_count_ch38 = rec_iters[1]->second.getCount();
			rssi_data.sample_count_ch39 = rec_iters[2]->second.getCount();

			rssi_data.rssi_ch37 = rec_iters[0]->second.getMean();
			rssi_data.rssi_ch38 = rec_iters[1]->second.getMean();
			rssi_data.rssi_ch39 = rec_iters[2]->second.getMean();

			rssi_data.variance_ch37 = getVarianceDescriptor(rec_iters[0]->second.getVariance());
			rssi_data.variance_ch38 = getVarianceDescriptor(rec_iters[1]->second.getVariance());
			rssi_data.variance_ch39 = getVarianceDescriptor(rec_iters[2]->second.getVariance());

			sendRssiDataOverMesh(&rssi_data);

			// delete entry from map
			for(auto i = 0; i < 3; ++i) {
				recorder_map[i].erase(rec_iters[i]);
				// this invalidates main_iter, so we _must_ return after deleting.
			}
			return Coroutine::delayMs(Settings.burst_period_ms);
		}
	} // ** end burst loop **

	RSSIDATATRACKER_LOGd("End of burst");

	// burst is finished, now we wait a little longer
	return Coroutine::delayMs(Settings.accumulation_period_ms);
}

// --------------- generating rssi data --------------

void RssiDataTracker::sendPingRequestOverMesh(){
	rssi_ping_message_t ping_msg;

	cs_mesh_msg_t msg_wrapper;
	msg_wrapper.type = CS_MESH_MODEL_TYPE_RSSI_PING;
	msg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msg_wrapper.urgency = CS_MESH_URGENCY_LOW;

	msg_wrapper.payload = reinterpret_cast<uint8_t*>(&ping_msg);
	msg_wrapper.size = sizeof(ping_msg);

	event_t msgevt(CS_TYPE::CMD_SEND_MESH_MSG, &msg_wrapper,
			sizeof(cs_mesh_msg_t));

	msgevt.dispatch();
}

void RssiDataTracker::sendPingResponseOverMesh(){
	cs_mesh_msg_t msg_wrapper;
	msg_wrapper.type = CS_MESH_MODEL_TYPE_CMD_NOOP;
	msg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msg_wrapper.urgency = CS_MESH_URGENCY_LOW;

	msg_wrapper.payload = nullptr;
	msg_wrapper.size = 0;

	event_t msgevt(CS_TYPE::CMD_SEND_MESH_MSG, &msg_wrapper,
			sizeof(cs_mesh_msg_t));

	msgevt.dispatch();
}

void RssiDataTracker::receivePingMessage(MeshMsgEvent& meshMsgEvent){
	if (meshMsgEvent.hops == 0) {
		sendPingResponseOverMesh();
	}
}

// ------------- communicating rssi data -------------

void RssiDataTracker::sendRssiDataOverMesh(rssi_data_message_t* rssi_data_message){
	cs_mesh_msg_t msg_wrapper;
	msg_wrapper.type = CS_MESH_MODEL_TYPE_RSSI_DATA;
	msg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msg_wrapper.urgency = CS_MESH_URGENCY_LOW;

	msg_wrapper.payload = reinterpret_cast<uint8_t*>(rssi_data_message);
	msg_wrapper.size = sizeof(*rssi_data_message);

	event_t msgevt(CS_TYPE::CMD_SEND_MESH_MSG, &msg_wrapper,
			sizeof(cs_mesh_msg_t));

	msgevt.dispatch();
}

void RssiDataTracker::sendRssiDataOverUart(rssi_data_message_t* rssi_data_message){
	UartHandler::getInstance().writeMsg(
			UART_OPCODE_TX_RSSI_DATA_MESSAGE,
			reinterpret_cast<uint8_t*>(rssi_data_message),
			sizeof(*rssi_data_message));
}

void RssiDataTracker::receiveRssiDataMessage(MeshMsgEvent& meshMsgEvent){
	auto& rssi_data_message =
			meshMsgEvent.getPacket<CS_MESH_MODEL_TYPE_RSSI_DATA>();
	sendRssiDataOverUart(&rssi_data_message);
}

// ------------- recording mesh messages -------------

void RssiDataTracker::receiveMeshMsgEvent(MeshMsgEvent& mesh_msg_evt) {
	if (mesh_msg_evt.hops == 0) { // TODO: 0 hops, or 1 hops?!
		RSSIDATATRACKER_LOGd("logging mesh msg event with 0 hops");
		recordRssiValue(
				mesh_msg_evt.srcAddress,
				mesh_msg_evt.rssi,
				mesh_msg_evt.channel
				);
	}

	// can't interpret the rssi value in this case.
}

void RssiDataTracker::handleEvent(event_t &evt) {
	if (flushRoutine.handleEvent(evt)) {
		return;
	}

	if (evt.type == CS_TYPE::EVT_RECV_MESH_MSG) {
		auto& meshMsgEvent = *UNTYPIFY(EVT_RECV_MESH_MSG, evt.getData());

		receiveMeshMsgEvent(meshMsgEvent);

		switch (meshMsgEvent.type) {
			case CS_MESH_MODEL_TYPE_RSSI_PING: {
				RSSIDATATRACKER_LOGd("received rssi ping message");
				receivePingMessage(meshMsgEvent);
				break;
			}
			case CS_MESH_MODEL_TYPE_RSSI_DATA: {
				RSSIDATATRACKER_LOGd("received rssi data message");
				receiveRssiDataMessage(meshMsgEvent);
				break;
			}
			default: {
				break;
			}
		}
	}

	return;
}
