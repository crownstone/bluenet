/**
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

extern "C" {
#include <protocol/rbc_mesh.h>
//#include <protocol/rbc_mesh_common.h>
//#include <protocol/led_config.h>

void rbc_mesh_event_handler(rbc_mesh_event_t* evt);

}

class CMesh {
private:
	// constructor is hidden from the user
	CMesh();

	// destructor is hidden from the user
	~CMesh();
	
	CMesh(CMesh const&); // singleton, deny implementation
	void operator=(CMesh const &); // singleton, deny implementation

public:
	// use static variant of singleton, no dynamic memory allocation
	static CMesh& getInstance() {
		static CMesh instance;
		return instance;
	}

	// initialize
	void init();

	// send message
	void send(uint8_t handle, uint32_t value);

	void send(uint8_t handle, void* p_data, uint8_t length);
	bool receive(uint8_t handle, void** p_data, uint16_t& length);

	// returns last received message
	uint32_t receive(uint8_t handle);

	// set callback to receive message
	void set_callback();
};

