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
#include <protocol/cs_MeshMessageTypes.h>
#include <mesh/cs_MeshControl.h>

extern "C" {

#include <rbc_mesh.h>
//#include <protocol/rbc_mesh_common.h>
//#include <protocol/led_config.h>

}

//! values are updated MESH_UPDATE_FREQUENCY times per second
#define MESH_UPDATE_FREQUENCY 10
//#define MESH_CLOCK_SOURCE (CLOCK_SOURCE)



/** Wrapper class around the mesh protocol files.
 */
class Mesh {
private:

	//! app timer id for tick function
#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;
#else
	uint32_t                 _appTimerId;
#endif

	bool                     _started;
	bool                     _running;

	bool                     _first[MESH_NUM_HANDLES];
	uint32_t                 _mesh_start_time = 0;

	uint32_t                 _messageCounter[MESH_NUM_HANDLES];

#if (NORDIC_SDK_VERSION >= 11) 
	static const nrf_clock_lf_cfg_t  meshClockSource;
#endif

	MeshControl&             _meshControl;

	bool                     _encryptionEnabled;

	//! constructor is hidden from the user
	Mesh();

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

	bool decodeMessage(encrypted_mesh_message_t* encoded, uint16_t encodedLength,
			mesh_message_t* decoded, uint16_t decodedLength);
	void encodeMessage(mesh_message_t* decoded, uint16_t decodedLength,
			encrypted_mesh_message_t* encoded, uint16_t encodedLength);

	void resolveConflict(uint8_t handle, encrypted_mesh_message_t* p_old, uint16_t length_old,
			encrypted_mesh_message_t* p_new, uint16_t length_new);

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

	//! resume the mesh, i.e. start mesh radio and handle incoming messages
	void resume();
	//! pause the mesh
	void pause();

	inline bool isRunning() { return _running && _started; }

	//! send message
	uint32_t send(uint8_t channel, void* p_data, uint8_t length);

	//! returns last message on channel
	bool getLastMessage(uint8_t channel, void* p_data, uint16_t& length);

};

