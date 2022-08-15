/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>

/**
 * Class that provides a dimmer.
 * - Determines whether to start on zero crossing.
 * - Controls dimmer enable GPIO.
 * - TODO: take care of synchronizing with net frequency
 */
class Dimmer {
public:
	void init(const boards_config_t& board);

	/**
	 * Returns true when this board has a dimmer.
	 */
	bool hasDimmer();

	/**
	 * Start dimmer.
	 *
	 * To be called once there is enough power to enable the dimmer.
	 */
	void start();

	/**
	 * Set dimmer intensity.
	 *
	 * @param[in] intensity       Intensity value to set: 0-100.
	 * @param[in] fade            Whether to fade towards the new intensity. False will set it immediately.
	 * @return true on success.
	 */
	bool set(uint8_t intensity, bool fade);

	/**
	 * Change the soft of speed.
	 *
	 * TODO: remove this function again once we have a nice default value.
	 */
	void setSoftOnSpeed(uint8_t speed);

private:
	uint32_t _hardwareBoard;
	uint8_t _pinEnableDimmer;
	bool _hasDimmer = false;

	TYPIFY(STATE_SOFT_ON_SPEED) _softOnSpeed;

	bool _initialized = false;
	bool _started     = false;
	bool _enabled     = false;

	void enable();
};
