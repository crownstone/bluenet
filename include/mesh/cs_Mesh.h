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

//! values are updated MESH_UPDATE_FREQUENCY times per second
#define MESH_UPDATE_FREQUENCY 10
//! number of channels used in the mesh
#define MESH_NUM_OF_CHANNELS 2
//! time given for boot up (every first message received on each channel
//! will be ignored during boot up)
#define BOOT_TIME 2000 // 2 seconds

//#define MESH_ACCESS_ADDR 0xA541A68E
#define MESH_ACCESS_ADDR 0xA641A69E
#define MESH_INTERVAL_MIN_MS 100
#define MESH_CHANNEL 38
#define MESH_CLOCK_SOURCE (CLOCK_SOURCE)



/** Wrapper class around the mesh protocol files.
 */
class Mesh {
private:

	//! app timer id for tick function
	app_timer_id_t           _appTimerId;

	bool started;

	bool                     _first[MESH_NUM_OF_CHANNELS];
	uint32_t                 _mesh_start_time = 0;

	//! constructor is hidden from the user
	Mesh();

	//! destructor is hidden from the user
	~Mesh();

	Mesh(Mesh const&); //! singleton, deny implementation
	void operator=(Mesh const &); //! singleton, deny implementation

	void tick();
	static void staticTick(Mesh* ptr) {
		ptr->tick();
	}

	void startTicking();
	void stopTicking();
	void scheduleNextTick();

	void checkForMessages();
	void handleMeshMessage(rbc_mesh_event_t* evt);


public:
	//! use static variant of singleton, no dynamic memory allocation
	static Mesh& getInstance() {
		static Mesh instance;
		return instance;
	}

	//! initialize
	void init();

	//! start the mesh, i.e. start mesh radio and handle incoming messages
	void start();
	//! stop the mesh
	void stop();
	//! restart the mesh, i.e. on disconnect to start advertising
	void restart();

	inline bool isRunning() { return started; }

	//! send message
	void send(uint8_t channel, void* p_data, uint8_t length);

	//! returns last message on channel
	bool getLastMessage(uint8_t channel, void** p_data, uint16_t& length);

};

