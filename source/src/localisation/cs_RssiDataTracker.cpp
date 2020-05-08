
#include <localisation/cs_RssiDataTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <events/cs_Event.h>
#include <time/cs_SystemTime.h>
#include <storage/cs_State.h>

#include <functional>

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


// coroutines
// note: tickrate currently is 10 ticks/s
class Coroutine{
public:
	uint32_t next_call_tickcount = 0;
	std::function<uint32_t(void)> routine;

	Coroutine(std::function<uint32_t(void)> r) : routine(r) {}

	void operator()(uint32_t current_tick_count){
		// not doing roll-over checks here yet..
		if(current_tick_count > next_call_tickcount){
			auto ticks_to_wait = routine();
			next_call_tickcount = current_tick_count + ticks_to_wait;
		}
	}
};

uint32_t loggingAction(){
	LOGd("RssiDataTracker tick happened");

	return 20;
}

uint32_t pingAction(){
	LOGd("RssiDataTracker sending ping");
	static uint8_t pingcounter = 0;

	// details with (0) need to be filled in by primary sender RssiDataTracker.
	// details with (1) will be filled in by primary recipient MeshMsgHandler.
	// details with (2) will be filled in by primary recipient RssiDataTracker.
	static rssi_ping_message_t pingmsg = {0};
	pingmsg.sample_id = pingcounter++; // (0)
	pingmsg.rssi = 0xff;               // (1)
	pingmsg.sender_id = 0xff;	       // (1)
	pingmsg.recipient_id = 0xff;       // (2)

	sendPingMsg(&pingmsg);

	return 50;
}

Coroutine loggingRoutine (loggingAction);
Coroutine pingRoutine (pingAction);




// RssiDataTracker methods.

void RssiDataTracker::init(){
	 State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &my_id, sizeof(my_id));
	 LOGw("RssiDataTracker: my_id %d",my_id);
}


void RssiDataTracker::forwardPingMsgOverMesh(rssi_ping_message_t* primary_ping_msg){
	LOGd("lets forward this ping msg over mesh");
	primary_ping_msg->recipient_id = my_id;
	sendPingMsg(primary_ping_msg);
}

void RssiDataTracker::forwardPingMsgToTestSuite(rssi_ping_message_t* secondary_ping_msg){
	LOGd("lets forward this ping msg over mesh");
	printPingMsg(secondary_ping_msg);
}


uint32_t pingcounter = 0;
void RssiDataTracker::handleEvent(event_t& evt){
	switch(evt.type){
	case CS_TYPE::EVT_TICK: {
		auto current_tick_count = *reinterpret_cast<uint32_t*>(evt.data);

		loggingRoutine(current_tick_count);
		pingRoutine(current_tick_count);

		break;
	}

	case CS_TYPE::EVT_MESH_RSSI_PING: {
		auto pingmsg_ptr = reinterpret_cast<rssi_ping_message_t*>(evt.data);
		LOGd("pingcounter: %d ", pingcounter++);
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
