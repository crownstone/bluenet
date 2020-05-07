
#include <localisation/cs_RssiDataTracker.h>

#include <drivers/cs_serial.h>
#include <events/cs_event.h>

void handleEvent(event_t& evt){
	switch(evt.type){
	case EVT_MESH_RSSI_PING:
		LOGd("got a ping message!");
	default:
		return;
	}

	return;
}
