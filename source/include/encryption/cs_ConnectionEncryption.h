/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 21, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_Packets.h>
#include <structs/cs_PacketsInternal.h>

#define DEFAULT_VALIDATION_KEY  0xCAFEBABE

enum class ConnectionEncryptionType {
	CTR,
	ECB
};

class ConnectionEncryption : EventListener {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static ConnectionEncryption& getInstance() {
		static ConnectionEncryption instance;
		return instance;
	}

	/**
	 * Initialize the class, reads from State.
	 */
	void init();

	/**
	 * Add headers and encrypt data.
	 *
	 * @param[in]  input               Input data to be encrypted.
	 * @param[out] output              Buffer to write headers and encrypted data to.
	 * @param[in]  accessLevel         Access level to use for encryption.
	 * @param[in]  encryptionType      Type of encryption to use.
	 * @return                         Return code.
	 */
	cs_ret_code_t encrypt(cs_data_t input, cs_data_t output, EncryptionAccessLevel accessLevel, ConnectionEncryptionType encryptionType);

	/**
	 * Parses headers, decrypts data, and validates decrypted data.
	 *
	 * @param[in]  input               Input data to be parsed and decrypted.
	 * @param[out] output              Buffer to write decrypted payload to.
	 * @param[out] accessLevel         Access level that was used for encryption.
	 * @param[in]  encryptionType      Type of encryption to use.
	 * @return                         Return code.
	 */
	cs_ret_code_t decrypt(cs_data_t input, cs_data_t output, EncryptionAccessLevel& accessLevel, ConnectionEncryptionType encryptionType);

	/**
	 * Get the required output buffer size when encrypting plaintext.
	 *
	 * @param[in]  plaintextBufferSize      Size of the buffer with the payload that will be encrypted.
	 * @param[in]  encryptionType           Type of encryption to use.
	 * @return                              Required size of the buffer that will hold the encryption headers and encrypted payload.
	 */
	static cs_buffer_size_t getEncryptedBufferSize(cs_buffer_size_t plaintextBufferSize, ConnectionEncryptionType encryptionType);

	/**
	 * Get the required output buffer size when decrypting encrypted data.
	 *
	 * @param[in]  encryptedBufferSize      Size of the buffer with encryption headers and encrypted payload.
	 * @param[in]  encryptionType           Type of encryption to use.
	 * @return                              Required size of the buffer that will hold the decrypted payload.
	 */
	static cs_buffer_size_t getPlaintextBufferSize(cs_buffer_size_t encryptedBufferSize, ConnectionEncryptionType encryptionType);

	/**
	 * Set session data. To be used when connecting to another crownstone.
	 *
	 * @param[in]  sessionData              The session data that has been read.
	 *
	 * @return ERR_PROTOCOL_UNSUPPORTED     When the protocol is not supported.
	 * @return ERR_SUCCESS                  When the session data is set.
	 */
	cs_ret_code_t setSessionData(session_data_t& sessionData);

	/**
	 * Whether a it's allowed to write to a characteristic.
	 *
	 * TODO: remove and let stack handle this.
	 */
	bool allowedToWrite();

	/**
	 * Close connection due to insufficient access.
	 */
	void disconnect();

	/**
	 * Handle events.
	 */
	void handleEvent(event_t& event) override;

private:
	// This class is singleton, make constructor private.
	ConnectionEncryption();

	// This class is singleton, deny implementation
	ConnectionEncryption(ConnectionEncryption const&);

	// This class is singleton, deny implementation
	void operator=(ConnectionEncryption const &);

	/**
	 * Session data: data that's valid for a whole session (connection).
	 */
	session_data_t _sessionData;

	/**
	 * Nonce used for encryption and decryption.
	 *
	 * Cached because it holds the session nonce.
	 */
	encryption_nonce_t _nonce;

	/**
	 * Generate new session data.
	 *
	 * To be called on connect.
	 */
	void generateSessionData();
};

