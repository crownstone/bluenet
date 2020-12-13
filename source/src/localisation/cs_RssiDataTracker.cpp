#include <localisation/cs_RssiDataTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <events/cs_Event.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <test/cs_Test.h>

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

// ------------ Sending ping stuff ------------


uint32_t RssiDataTracker::flushAggregatedRssiData() {
	RSSIDATATRACKER_LOGd("flushAggregatedRssiData");
	// start flushing phase, here we wait quite a bit shorter until the map is empty.

//	for(auto iter = std::begin(variance_recorders);
//			iter != std::end(variance_recorders); iter++) {
//		StonePair stone_pair = iter->first;
//		OnlineVarianceRecorder recorder = iter->second;
//
//		if (recorder.getCount() >= min_samples_to_trigger_burst) {
//
//			uint32_t mean = static_cast<uint32_t>(recorder.getMean());
//			RSSIDATATRACKER_LOGv("flushing maps from %d: {send:%d recv:%d rssi:%d}",
//					my_id, stone_pair.first, stone_pair.second, mean);
//
//			rssi_ping_message_t ping_msg;
//			ping_msg.sender_id = stone_pair.first;
//			ping_msg.recipient_id = stone_pair.second;
//			ping_msg.rssi = mean;
//			ping_msg.channel = 0;
//			ping_msg.sample_id = std::max(0xff,recorder.getCount());
//
//
//			sendSecondaryPingMsg(&ping_msg);
//			variance_recorders.erase(iter);
//
//			return Coroutine::delayMs(burst_period_ms);
//		}
//	}

	// start accumulation phase: just wait very long
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
	msg_wrapper.size = sizeof(rssi_data_message);

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
