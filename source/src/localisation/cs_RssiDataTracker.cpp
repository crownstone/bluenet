
#include <localisation/cs_RssiDataTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <events/cs_Event.h>

#include <functional>

uint32_t next_log_msg_tickcount = 0;

// returns number of ticks to wait. currently 100 ticks/s
uint32_t loggingAction(){
	LOGd("RssiDataTracker tick happened");

	return 50;
}

uint32_t pingAction(){
	LOGd("RssiDataTracker sending ping");

	return 20;
}

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

Coroutine loggingRoutine (loggingAction);
Coroutine pingRoutine (pingAction);

void RssiDataTracker::handleEvent(event_t& evt){
	switch(evt.type){
	case CS_TYPE::EVT_TICK: {
		auto current_tick_count = *reinterpret_cast<uint32_t*>(evt.data);

		loggingRoutine(current_tick_count);
		pingRoutine(current_tick_count);

		break;
	}

	case CS_TYPE::EVT_MESH_RSSI_PING: {
		LOGd("got a ping message!");
		break;
	}
	default:
		break;
	}

	return;
}
