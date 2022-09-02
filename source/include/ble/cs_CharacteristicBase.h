/**
 * @file
 * Bluetooth Low Energy characteristics.
 *
 * @authors Crownstone Team, Christopher Mason
 * @copyright Crownstone B.V.
 * @date Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_Service.h>
#include <common/cs_Types.h>
#include <third/std/function.h>
#include <structs/cs_CharacteristicStructs.h>

#define MAX_NOTIFICATION_LEN 20

class Service;
class CharacteristicBase;
typedef function<void(CharacteristicEventType, CharacteristicBase*, EncryptionAccessLevel)> characteristic_callback_t;

/**
 * Base interface class for characteristics.
 */
class CharacteristicBase {
public:
	CharacteristicBase();

	virtual ~CharacteristicBase() {}

	/**
	 * Set the name of this characteristic.
	 *
	 * You must keep the string in memory.
	 * This is already the case when using a string literal: setName("myName");
	 */
	cs_ret_code_t setName(const char* const name);

	/**
	 * Set the UUID of this characteristic.
	 *
	 * Must be done before init.
	 * The service UUID will be used as base.
	 */
	cs_ret_code_t setUuid(uint16_t uuid);

	/**
	 * Set options for this characteristic.
	 * Must be done before init.
	 */
	cs_ret_code_t setOptions(const characteristic_options_t& options);

	/**
	 * Register an event handler.
	 * Must be done before init.
	 */
	cs_ret_code_t setEventHandler(const characteristic_callback_t& closure);

	/**
	 * Set the buffer to be used for the (plain text) characteristic value.
	 * Must be done before init.
	 */
	cs_ret_code_t setValueBuffer(buffer_ptr_t buffer, cs_buffer_size_t size);

	/**
	 * Set the initial value length.
	 * Must be done before init.
	 */
	cs_ret_code_t setInitialValueLength(cs_buffer_size_t size);

	/**
	 * Initialize the characteristic: add it to the softdevice.
	 *
	 * Should be done after configuring name, permissions, value, etc.
	 */
	cs_ret_code_t init(Service* svc);

	/**
	 * Call this function when you changed the characteristic value.
	 *
	 * @param[in] length     The new length of the characteristic value.
	 */
	cs_ret_code_t updateValue(uint16_t length);

	/**
	 * Notify or indicate the characteristic value.
	 *
	 * When using the notification chunker, this will start sending the value in chunks.
	 *
	 * @param[in] length     Number of bytes to send. Use 0 to send min(value size, max notification size) bytes.
	 * @param[in] offset     Offset in bytes of the value to send.
	 */
	cs_ret_code_t notify(uint16_t length = 0, uint16_t offset = 0);

	cs_data_t getValue();

	uint16_t getValueHandle();

	uint16_t getCccdHandle();

protected:
	friend Service;

	/**
	 * Function to be called by the stack when this characteristic is written over BLE.
	 */
	void onWrite(uint16_t length);

	/**
	 * Function to be called by the stack when the notification or indication has been sent.
	 */
	void onNotificationDone();

	/**
	 * Function to be called by the stack when this characteristic configuration is written over BLE.
	 */
	void onCccdWrite(const uint8_t* data, uint16_t size);

private:
	struct __attribute__((__packed__)) notification_t {
		uint8_t partNr;
		uint8_t data[MAX_NOTIFICATION_LEN - 1];
	};

	//! Whether this characteristic has been initialized.
	bool _initialized = false;

	//! Name of this characteristic.
	const char* _name = nullptr;

	//! UUID of this characteristic.
	uint16_t _uuid = 0;

	//! The configured options of the characteristic.
	characteristic_options_t _options = characteristic_options_t();

	//! The callback to call.
	characteristic_callback_t _callback = {};

	//! Handles, set by softdevice at init.
	ble_gatts_char_handles_t _handles = ble_gatts_char_handles_t();

	//! Reference to corresponding service, set at init.
	Service* _service = nullptr;


	/**
	 * The buffer holding the (plain text) characteristic value.
	 */
	cs_data_t _buffer = {};

	//! Actual length of (plain text) data stored in the buffer.
	uint16_t _valueLength = 0;

	/**
	 * The buffer holding the encrypted characteristic value.
	 */
	cs_data_t _encryptedBuffer = {};

	//! Actual length of the (encrypted) data stored in the buffer.
	uint16_t _encryptedValueLength = 0;

	/**
	 * Buffer used for chunked notifications.
	 */
	notification_t* _notificationBuffer = nullptr;

	//! Flag to indicate if notification or indication is pending to be sent.
	bool _notificationPending = false;

	//! When using the notification chunker, this is the current offset of the value to notify next.
	uint16_t _notificationOffset = 0;

	//! Whether the central subscribed for notifications.
	bool _subscribedForNotifications = false;

	//! Whether the central subscribed for indication.
	bool _subscribedForIndications = false;

	/**
	 * Initialize the encrypted buffer.
	 *
	 * Allocates a buffer if no shared encrypted buffer is used.
	 * Checks if the encrypted buffer is of correct size.
	 */
	cs_ret_code_t initEncryptedBuffer();

	/**
	 * Deallocates the encrypted buffer if it was allocated.
	 */
	void deinitEncryptedBuffer();

	/**
	 * Update the gatt value in the softdevice.
	 */
	cs_ret_code_t setGattValue();

	cs_ret_code_t notifyMultipart();

	/**
	 * Return the maximum length of the value used by the GATT server.
	 * With encryption, this is the maximum length that the value can be when encrypted.
	 * Without encryption, this is the same as the buffer size.
	 */
	uint16_t getGattValueMaxLength();

	/**
	 * Return the actual length of the value used by the GATT server.
	 * With encryption, this is the length of the encrypted value.
	 * Without encryption, this is the same as the value size.
	 */
	uint16_t getGattValueLength();

	/**
	 * Return the pointer to the memory where the value is accessed by the GATT server.
	 * With encryption, this is the encrypted buffer.
	 * Without encryption, this is the value buffer.
	 */
	uint8_t* getGattValue();



	bool isEncrypted();

};
