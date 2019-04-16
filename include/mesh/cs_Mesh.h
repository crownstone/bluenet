/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

extern "C" {
#include <generic_onoff_client.h>
#include <nrf_mesh_config_app.h>
}

class Mesh {
public:
	/**
	 * Get a reference to the State object.
	 */
	static Mesh& getInstance();

	/**
	 * Init the mesh.
	 */
	void init();

	/**
	 * Start the mesh.
	 *
	 * Start using the radio and handle incoming messages.
	 */
	void start();

	/**
	 * Stop the mesh.
	 *
	 * Stops all radio usage.
	 */
	void stop();

	/**
	 * Internal usage
	 */
	static void staticModelsInitCallback() {
		Mesh::getInstance().modelsInitCallback();
	}
	void modelsInitCallback();

private:
	//! State constructor, singleton, thus made private
	Mesh();

	//! State copy constructor, singleton, thus made private
	Mesh(Mesh const&);

	//! Assignment operator, singleton, thus made private
	void operator=(Mesh const &);

	generic_onoff_client_t _clients[CLIENT_MODEL_INSTANCE_COUNT];
	bool _isProvisioned = false;
};
