/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_Packets.h>
#include <common/cs_Types.h>

/**
 * Class to get keys based on access level, and to check access levels.
 *
 * - Generates temporary keys.
 * - Caches state.
 */
class KeysAndAccess {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static KeysAndAccess& getInstance() {
		static KeysAndAccess instance;
		return instance;
	}

	/**
	 * Init the class: reads from State.
	 */
	void init();

	/**
	 * Verify if the access level provided is sufficient
	 *
	 * @param[in] minimum              Minimum required access level.
	 * @param[in] provided             Provided access level.
	 * @return                         True when provided access level meets the minimum required level.
	 */
	bool allowAccess(EncryptionAccessLevel minimum, EncryptionAccessLevel provided);

	/**
	 * Get key of given access level.
	 *
	 * @param[in] accessLevel          Access level of which to get the key.
	 * @param[out] outBuf              Buffer to write the key to.
	 * @param[in] outBufSize           Size of the buffer.
	 * @return                         True on success.
	 */
	bool getKey(EncryptionAccessLevel accessLevel, buffer_ptr_t outBuf, cs_buffer_size_t outBufSize);

	/**
	 * Get a pointer to the setup key.
	 *
	 * @return                         Pointer and size of key. Pointer can be NULL when setup key is unavailable.
	 */
	cs_data_t getSetupKey();

	/**
	 * Generate a new setup key.
	 *
	 * Will only do so if operation mode is setup.
	 */
	void generateSetupKey();

	/**
	 * Set the setup key. To be used when connecting to another crownstone.
	 *
	 * @param[in] data                      The key that has been read.
	 *
	 * @return ERR_WRONG_PAYLOAD_LENGTH     When the data is of incorrect size.
	 * @return ERR_SUCCESS                  When the setup key is set.
	 */
	cs_ret_code_t setSetupKey(cs_data_t data);

	/**
	 * Invalidate the setup key.
	 */
	void invalidateSetupKey();

private:
	// This class is singleton, make constructor private.
	KeysAndAccess();

	// This class is singleton, deny implementation
	KeysAndAccess(KeysAndAccess const&);

	// This class is singleton, deny implementation
	void operator=(KeysAndAccess const &);

	/**
	 * Cached operation mode.
	 */
	OperationMode _operationMode = OperationMode::OPERATION_MODE_UNINITIALIZED;

	/**
	 * Cache of whether encryption is enabled.
	 */
	bool _encryptionEnabled = true;

	/**
	 * Current setup key.
	 */
	uint8_t _setupKey[ENCRYPTION_KEY_LENGTH];

	/**
	 * Whether the setup key is valid.
	 */
	bool _setupKeyValid = false;

};


