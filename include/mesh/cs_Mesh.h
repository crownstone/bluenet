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
#include <protocol/mesh/cs_MeshMessageCounter.h>

extern "C" {

#include <rbc_mesh.h>
//#include <protocol/rbc_mesh_common.h>
//#include <protocol/led_config.h>

}

//! values are updated MESH_UPDATE_FREQUENCY times per second
#define MESH_UPDATE_FREQUENCY 10
//#define MESH_CLOCK_SOURCE (CLOCK_SOURCE)

typedef rbc_mesh_value_handle_t mesh_handle_t;


/** Wrapper class around the mesh protocol files.
 */
class Mesh {
private:

	//! app timer id for tick function
	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;

	bool                     _initialized;
	bool                     _started;
	bool                     _running;

	//! Keeps up the message counters for each handle
	MeshMessageCounter       _messageCounter[MESH_HANDLE_COUNT];

	//! Keeps up when the mesh was started
	uint32_t                 _mesh_start_time = 0;

	static const nrf_clock_lf_cfg_t meshClockSource;

	static const uint16_t meshHandles[MESH_HANDLE_COUNT];

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

	//! Decrypts a message
	//! Copies from encoded to decoded msg.
	bool decodeMessage(encrypted_mesh_message_t* encoded, uint16_t encodedLength,
			mesh_message_t* decoded, uint16_t decodedLength);

	//! Encrypts a message
	//! Copies from decoded to encoded msg.
	bool encodeMessage(mesh_message_t* decoded, uint16_t decodedLength,
			encrypted_mesh_message_t* encoded, uint16_t encodedLength);

	//! Allocates 2 mesh_message_t msgs, decrypts the two messages to those structs.
	//! For the reply channel: send the one with the highest message counter, merge when equal.
	//! For the state change and broadcast channels: merge

	//! TODO: split up in a function per handle
	void resolveConflict(mesh_handle_t handle, encrypted_mesh_message_t* p_old, uint16_t length_old,
			encrypted_mesh_message_t* p_new, uint16_t length_new);

	//! Returns the message counter of a given handle
	//! Does not check if handle is valid!
	MeshMessageCounter& getMessageCounter(mesh_handle_t handle);

	//! Returns the message counter of a given handle index
	//! Returns NULL for an invalid handle index
	MeshMessageCounter& getMessageCounterFromIndex(uint16_t handleIndex);

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

	//! Returns whether the mesh is currently started and running.
	bool isRunning();

	//! Returns the index of a given handle. Returns INVALID_HANDLE if it has no index.
	uint16_t getHandleIndex(mesh_handle_t handle);


	//! Send message
	//! This allocates a mesh_message_t on stack, and copies given data to it
	//!   then allocates an encrypted_mesh_message_t and encrypts the message with encodeMessage()
	//!   then sets the value of that handle in the rbc_mesh.
	//! Returns message counter (or 0 when failed).
	//! TODO: can we not do so much copying and allocating?
	uint32_t send(mesh_handle_t handle, void* p_data, uint16_t length);

	//! Returns last message on channel
	//! p_data should be a pointer to allocated data, length the size of allocated.
	//! This allocates an encrypted_mesh_message_t, then allocates a mesh_message_t and decrypts to that.
	//! The resulting message is then copied to p_data.
	//! TODO: have a better way to do this
	bool getLastMessage(mesh_handle_t handle, void* p_data, uint16_t length);

};

