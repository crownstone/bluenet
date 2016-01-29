/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

/**
 * TODO: Build up the structure with servers and peripheral roles such that we can reach everybody from every position.
 */

#pragma once

//#include <common/cs_Types.h>

#include <drivers/cs_Timer.h>

extern "C" {

#include <protocol/rbc_mesh.h>
//#include <protocol/rbc_mesh_common.h>
//#include <protocol/led_config.h>

}

// values are updated MESH_UPDATE_FREQUENCY times per second
#define MESH_UPDATE_FREQUENCY 10
// number of channels used in the mesh
#define MESH_NUM_OF_CHANNELS 2
// time given for boot up (every first message received on each channel
// will be ignored during boot up)
#define BOOT_TIME 1000 // 1 second

#define MESH_ACCESS_ADDR 0xA541A68E
//#define MESH_ACCESS_ADDR 0xA641A69E
#define MESH_INTERVAL_MIN_MS 100
#define MESH_CHANNEL 38
#define MESH_CLOCK_SOURCE (CLOCK_SOURCE)



class CMesh {
private:

	// app timer id for tick function
	uint32_t				 _appTimerId;

	bool                     _first[MESH_NUM_OF_CHANNELS];
	uint32_t                 _mesh_init_time = 0;

	// constructor is hidden from the user
	CMesh();

	// destructor is hidden from the user
	~CMesh();
	
	CMesh(CMesh const&); // singleton, deny implementation
	void operator=(CMesh const &); // singleton, deny implementation

	void tick();
	static void staticTick(CMesh* ptr) {
		ptr->tick();
	}

	void scheduleNextTick();

	void checkForMessages();
	void handleMeshMessage(rbc_mesh_event_t* evt);

public:
	// use static variant of singleton, no dynamic memory allocation
	static CMesh& getInstance() {
		static CMesh instance;
		return instance;
	}

	// initialize
	void init();

	void startTicking() { Timer::getInstance().start(_appTimerId, APP_TIMER_TICKS(1, APP_TIMER_PRESCALER), this); };
	void stopTicking() { Timer::getInstance().stop(_appTimerId); };

	// send message
	void send(uint8_t channel, void* p_data, uint8_t length);

	// returns last message on channel
	bool getLastMessage(uint8_t channel, void** p_data, uint16_t& length);

};

