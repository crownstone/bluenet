/*
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

void rbc_mesh_event_handler(rbc_mesh_event_t* evt);

}

#define MESH_UPDATE_FREQUENCY 10

/** Wrapper class around the mesh protocol files.
 */
class CMesh {
private:

	//! app timer id for tick function
	uint32_t				 _appTimerId;

	//! constructor is hidden from the user
	CMesh();

	//! destructor is hidden from the user
	~CMesh();

	CMesh(CMesh const&); //! singleton, deny implementation
	void operator=(CMesh const &); //! singleton, deny implementation

	void tick();
	static void staticTick(CMesh* ptr) {
		ptr->tick();
	}

	void scheduleNextTick();

	void handleMeshReceive();

public:
	//! use static variant of singleton, no dynamic memory allocation
	static CMesh& getInstance() {
		static CMesh instance;
		return instance;
	}

	//! initialize
	void init();

	void startTicking() { Timer::getInstance().start(_appTimerId, APP_TIMER_TICKS(1, APP_TIMER_PRESCALER), this); };
	void stopTicking() { Timer::getInstance().stop(_appTimerId); };

	//! send message
//	void send(uint8_t handle, uint32_t value);

	void send(uint8_t channel, void* p_data, uint8_t length);

	//! returns last message on channel
	bool getLastMessage(uint8_t channel, void** p_data, uint16_t& length);

};

