/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <stdint.h>

class MeshControl {
private:
	MeshControl();
	MeshControl(MeshControl const&); // singleton, deny implementation
	void operator=(MeshControl const &); // singleton, deny implementation
public:
	// use static variant of singelton, no dynamic memory allocation
	static MeshControl& getInstance() {
		static MeshControl instance;
		return instance;
	}

	/**
	 * Get incoming messages and perform certain actions.
	 */
	void process(uint8_t channel, uint32_t message);

};
