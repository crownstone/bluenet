
#include <localisation/cs_RssiDataTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <events/cs_Event.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <test/cs_Test.h>

// some temporary/debug globals
uint32_t next_log_msg_tickcount = 0;
uint32_t prev_uptime_sec = 0;
uint32_t pings_received_this_sec = 0;

// utilities

void printPingMsg(rssi_ping_message_t* p){
	if(!p){
		return;
	}

	LOGd("ping: %d -> %d @ %d dB sample(#%d)",
			p->sender_id,
			p->recipient_id,
			p->rssi,
			p->sample_id);
}

void sendPingMsg(rssi_ping_message_t* pingmsg){
	cs_mesh_msg_t pingmsg_wrapper;
	pingmsg_wrapper.type = CS_MESH_MODEL_TYPE_RSSI_PING;
	pingmsg_wrapper.payload = reinterpret_cast<uint8_t*>(pingmsg);
	pingmsg_wrapper.size  = sizeof(rssi_ping_message_t);
	pingmsg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
	pingmsg_wrapper.urgency = CS_MESH_URGENCY_LOW;

	event_t pingmsgevt(CS_TYPE::CMD_SEND_MESH_MSG, &pingmsg_wrapper, sizeof(cs_mesh_msg_t));
	pingmsgevt.dispatch();
}

// ------------ RssiDataTracker methods ------------

RssiDataTracker::RssiDataTracker() :
		pingRoutine ([this](){ return sendPrimaryPingMessage(); }) {
}

void RssiDataTracker::init(){
	 State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &my_id, sizeof(my_id));
	 LOGw("RssiDataTracker: my_id %d",my_id);
}

// ------------ Ping stuff ------------

uint32_t RssiDataTracker::sendPrimaryPingMessage(){
	LOGd("RssiDataTracker sending ping for my_id(%d)", my_id);

	// details with (0) need to be filled in by primary sender RssiDataTracker.
	// details with (1) will be filled in by primary recipient MeshMsgHandler.
	// details with (2) will be filled in by primary recipient RssiDataTracker.
	static rssi_ping_message_t pingmsg = {0};
	pingmsg.sample_id = ping_sample_index++; // (0)
	pingmsg.rssi = 0xff;                     // (1)
	pingmsg.sender_id = 0xff;	             // (1)
	pingmsg.recipient_id = 0xff;             // (2)

	sendPingMsg(&pingmsg);

	return 50;
}

void RssiDataTracker::forwardPingMsgOverMesh(rssi_ping_message_t* primary_ping_msg){
	primary_ping_msg->recipient_id = my_id;
	sendPingMsg(primary_ping_msg);
}

void RssiDataTracker::forwardPingMsgToTestSuite(rssi_ping_message_t* secondary_ping_msg){
	LOGd("lets forward this ping msg to host");
	printPingMsg(secondary_ping_msg);
	char expressionstring[50];

	sprintf(expressionstring, "rssi: %d->%d",
			secondary_ping_msg->sender_id,
			secondary_ping_msg->recipient_id);

	TEST_PUSH_EXPR_D(this, expressionstring, secondary_ping_msg->rssi);

}


uint32_t received_pingcounter = 0;
void RssiDataTracker::handleEvent(event_t& evt){
	switch(evt.type){
	case CS_TYPE::EVT_TICK: {
		auto current_tick_count = *reinterpret_cast<uint32_t*>(evt.data);

		pingRoutine(current_tick_count);

		break;
	}

	case CS_TYPE::EVT_MESH_RSSI_PING: {
		auto pingmsg_ptr = reinterpret_cast<rssi_ping_message_t*>(evt.data);
		LOGd("incoming pingcounter: %d ", received_pingcounter++);
		if(pingmsg_ptr->recipient_id == 0xff){
			forwardPingMsgOverMesh(pingmsg_ptr);
		} else {
			forwardPingMsgToTestSuite(pingmsg_ptr);
		}

		break;
	}
	default:
		break;
	}

	return;
}
