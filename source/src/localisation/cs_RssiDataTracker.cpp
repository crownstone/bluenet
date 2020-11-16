#include <localisation/cs_RssiDataTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <events/cs_Event.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <test/cs_Test.h>

#define RSSIDATATRACKER_LOGd LOGd
#define RSSIDATATRACKER_LOGv LOGnone

// utilities

void printPingMsg(rssi_ping_message_t *p) {
	if (!p) {
		return;
	}

	RSSIDATATRACKER_LOGv("ping: %d -> %d @ %d dB sample(#%d)",
			p->sender_id,
			p->recipient_id,
			p->rssi,
			p->sample_id);
}

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

void RssiDataTracker::recordPingMsg(rssi_ping_message_t *ping_msg) {
	// TODO: size constraint... map may keep up to 256**2 items. when
	// 256 crownstones are on the same mesh.
	RSSIDATATRACKER_LOGv("record new sampleid (%d -> %d) %d",
			ping_msg->sender_id, ping_msg->recipient_id, ping_msg->sample_id);

	auto stone_pair = getKey(ping_msg);
	recordSampleId(stone_pair, ping_msg->sample_id);
	recordRssiValue(stone_pair, ping_msg->rssi);
}

void RssiDataTracker::recordSampleId(StonePair stone_pair, uint8_t sample_id) {
	last_received_sample_indices[stone_pair] = sample_id;
}

void RssiDataTracker::recordRssiValue(StonePair stone_pair, int8_t rssi) {
	auto var_rec_iter = variance_recorders.find(stone_pair);

	if (var_rec_iter == variance_recorders.end()) {
		OnlineVarianceRecorder ovr;
		variance_recorders[stone_pair] = ovr;
	} else if (var_rec_iter->second.isNumericPrecisionLow()) {
		var_rec_iter->second.reduceCount();
	}

	last_received_rssi[stone_pair] = rssi;
	variance_recorders[stone_pair].addValue(rssi);
}

// ------------ Sending ping stuff ------------
void RssiDataTracker::sendPingMsg(rssi_ping_message_t *ping_msg) {
	cs_mesh_msg_t pingmsg_wrapper;
	pingmsg_wrapper.type = CS_MESH_MODEL_TYPE_RSSI_PING;
	pingmsg_wrapper.payload = reinterpret_cast<uint8_t*>(ping_msg);
	pingmsg_wrapper.size = sizeof(rssi_ping_message_t);
	pingmsg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
	pingmsg_wrapper.urgency = CS_MESH_URGENCY_LOW;

	event_t pingmsgevt(CS_TYPE::CMD_SEND_MESH_MSG, &pingmsg_wrapper,
	        sizeof(cs_mesh_msg_t));
	pingmsgevt.dispatch();
}

void RssiDataTracker::sendPrimaryPingMessage() {
	RSSIDATATRACKER_LOGv("RssiDataTracker sending ping for my_id(%d), sample#%d",
			my_id, ping_sample_index);

	// details with (0) need to be filled in by primary sender RssiDataTracker.
	// details with (1) will be filled in by primary recipient MeshMsgHandler.
	// details with (2) will be filled in by primary recipient RssiDataTracker.
	static rssi_ping_message_t ping_msg = { 0 };
	ping_msg.sample_id = ping_sample_index++; // (0)
	ping_msg.rssi = 0;                        // (1)
	ping_msg.sender_id = 0;	                  // (1)
	ping_msg.recipient_id = 0;                // (2)

	sendPingMsg(&ping_msg);
}

bool RssiDataTracker::sendSecondaryPingMsg(rssi_ping_message_t *ping_msg) {
	if (ping_msg == nullptr || ping_msg->rssi == 0 || ping_msg->sender_id == 0
	        || ping_msg->recipient_id == 0) {
		return false;
	}

	sendPingMsg(ping_msg);
	return true;
}

uint32_t RssiDataTracker::flushAggregatedRssiData() {
	RSSIDATATRACKER_LOGd("flushAggregatedRssiData");
	// start flushing phase, here we wait quite a bit shorter until the map is empty.

	auto iter = std::begin(variance_recorders);
	if (iter != std::end(variance_recorders)) {
		StonePair stone_pair = iter->first;
		OnlineVarianceRecorder recorder = iter->second;

		uint32_t mean = static_cast<uint32_t>(recorder.getMean());
		RSSIDATATRACKER_LOGd("flushing maps from %d: {send:%d recv:%d rssi:%d}",
				my_id, stone_pair.first, stone_pair.second, mean);

		rssi_ping_message_t ping_msg;
		ping_msg.sender_id = stone_pair.first;
		ping_msg.recipient_id = stone_pair.second;
		ping_msg.rssi = mean;

		sendSecondaryPingMsg(&ping_msg);
		variance_recorders.erase(iter);

		// note: if the mesh is very active, setting this delay higher is risky.
		// in that case, we might need to add a condition that we only flush
		// entries that have accumulated enough samples.
		return Coroutine::delayMs(5);
	}

	// start accumulation phase: just wait very long
	return Coroutine::delayMs(30 * 60 * 1000);
}

// ------------ Receiving ping stuff ------------

void RssiDataTracker::handlePrimaryPingMessage(rssi_ping_message_t *ping_msg) {
	if (ping_msg == nullptr) {
		return;
	}

	ping_msg->recipient_id = my_id; // needs to be filled in before filtering

	if (filterSampleIndex(ping_msg) == nullptr) {
		// already seen this one before, don't re-propagate.
		return;
	}

	// send back a copy to the mesh in order to spread
	// the information to all nodes
	sendSecondaryPingMsg(ping_msg);

	// primary ping messages will never be recorded as secondary messages
	// since they only get propagated once (TTL=1) and there is no loopback.
	recordPingMsg(ping_msg);
}

void RssiDataTracker::handleSecondaryPingMessage(
        rssi_ping_message_t *ping_msg) {
	if (filterSampleIndex(ping_msg) == nullptr) {
		return; // (also catches ping_msg == nullptr)
	}

	printPingMsg(ping_msg);

	// secondary ping messages have not yet been recorded.
	recordPingMsg(ping_msg);

	pushPingMsgToHost(ping_msg);
}

void RssiDataTracker::pushPingMsgToHost(rssi_ping_message_t *ping_msg) {
	// TODO: change to formal uart protocol now that the hub will use it.

	char expressionstring[50];

	// crude workaround: we're using '_' as separator since
	// the strings that are pushed to test suite are separated
	// on ',' and we're using this expressionstring to push
	// stuff of several different pairs of crownstones but want
	// to split that out on the host side.
	sprintf(expressionstring, "rssi_%d_%d_%d", ping_msg->sender_id,
	        ping_msg->recipient_id, ping_msg->channel);

	TEST_PUSH_EXPR_D(this, expressionstring, ping_msg->rssi);
}

void RssiDataTracker::handleGenericMeshMessage(MeshMsgEvent *mesh_msg_evt) {
	if (mesh_msg_evt == nullptr || mesh_msg_evt->hops > 1) {
		// can't interpret the rssi value in this case.
		return;
	}

	rssi_ping_message_t forged_ping_msg;
	forged_ping_msg.sample_id = 0x00;
	forged_ping_msg.recipient_id = my_id;
	forged_ping_msg.sender_id = mesh_msg_evt->srcAddress;
	forged_ping_msg.rssi = mesh_msg_evt->rssi;
	forged_ping_msg.channel = mesh_msg_evt->channel;

	auto stone_pair = getKey(&forged_ping_msg);
	recordRssiValue(stone_pair, forged_ping_msg.rssi);
}

rssi_ping_message_t* RssiDataTracker::filterSampleIndex(
        rssi_ping_message_t *p) {
	if (p == nullptr) {
		return nullptr;
	}

	auto p_key = getKey(p);

	if (last_received_sample_indices[p_key] == p->sample_id) {
		// sample index should be incremented each ping message,
		// a second (third...) message from crownstone_id with a previously
		// recorded sample_id is filtered out.
		RSSIDATATRACKER_LOGv("filtered out stale ping message (%d -> %d) %d",
				p->sender_id, p->recipient_id, p->sample_id);
		return nullptr;
	}

	return p;
}

uint32_t received_pingcounter = 0;
void RssiDataTracker::handleEvent(event_t &evt) {
	if (flushRoutine.handleEvent(evt)) {
		return;
	}

	switch (evt.type) {
		case CS_TYPE::EVT_MESH_RSSI_PING: {
			auto pingmsg_ptr = reinterpret_cast<rssi_ping_message_t*>(evt.data);
			RSSIDATATRACKER_LOGv("incoming pingcounter: %d ", received_pingcounter++);
			if (pingmsg_ptr->recipient_id == 0) {
				handlePrimaryPingMessage(pingmsg_ptr);
			} else {
				handleSecondaryPingMessage(pingmsg_ptr);
			}

			break;
		}

		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto meshMsgEvent = reinterpret_cast<MeshMsgEvent*>(evt.getData());
			handleGenericMeshMessage(meshMsgEvent);
			break;
		}

		default:
			break;
	}

	return;
}
