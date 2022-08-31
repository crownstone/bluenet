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
#include <ble/cs_UUID.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <common/cs_Types.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_KeysAndAccess.h>
#include <logging/cs_Logger.h>
#include <structs/buffer/cs_EncryptedBuffer.h>
#include <third/std/function.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#define LOGCharacteristicDebug LOGvv
#define LogLevelCharacteristicDebug SERIAL_VERY_VERBOSE

class Service;

#define MAX_NOTIFICATION_LEN 20

struct characteristic_options_t {
	//! Whether the characteristic is readable.
	bool read = false;

	//! Whether the characteristic can be written to with or without response.
	bool write = false;

	//! Whether the characteristic supports notifications and indications.
	bool notify = false;

	/**
	 * Whether the characteristic should automatically notify the value when you updated the value.
	 */
	bool autoNotify = false;

	/**
	 * Whether to notify the value in multiple notifications.
	 * This uses a bluenet specific protocol.
	 * This has a side effect that it changes the (encrypted) value of this characteristic.
	 * So the connected device won't be able to read the (encrypted) value anymore, it has to be done via notifications.
	 *
	 * When not using this option, autoNotify will only send the first MAX_NOTIFICATION_LEN bytes.
	 *
	 * When using a shared buffer, the data on this buffer should not be changed until all notifications are sent.
	 */
	bool notificationChunker = false;

	//! Whether to encrypt the characteristic value.
	bool encrypted = false;

	/**
	 * Whether to use a shared encryption buffer.
	 * If false, a buffer is allocated for the characteristic
	 * If true, the global EncryptionBuffer is used.
	 * This option has no effect when encryption is disabled.
	 */
	bool sharedEncryptionBuffer = false;

	/**
	 * The minimum encryption level to be used for this characteristic.
	 *
	 * If the characteristic value is set, it will encrypt itself with the key of this access level.
	 * If the characteristic value was written by the central, the user level must be at least this access level.
	 */
	EncryptionAccessLevel minAccessLevel = ADMIN;

	/**
	 * Type of encryption to use.
	 */
	ConnectionEncryptionType encryptionType = ConnectionEncryptionType::CTR;
};

enum CharacteristicEventType {
	/**
	 * The characteristic value was written by the connected device.
	 * With encryption, the access level is set to the access level of the connected device.
	 */
	CHARACTERISTIC_EVENT_WRITE,

	//! The characteristic value was read by the connected device.
	CHARACTERISTIC_EVENT_READ,

	//! The connected device subscribed or unsubscribed for notifications or indications.
	CHARACTERISTIC_EVENT_SUBSCRIPTION,

	//! The notification of indication was sent.
	CHARACTERISTIC_EVENT_NOTIFY_DONE,
};

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
	cs_ret_code_t setUUID(uint16_t uuid);

	/**
	 * Set options for this characteristic.
	 * Must be done before init.
	 */
	cs_ret_code_t setOptions(const characteristic_options_t& options);

	/**
	 * Register an event handler.
	 * Must be done before init.
	 */
	cs_ret_code_t registerEventHandler(const characteristic_callback_t& closure);

	/**
	 * Set the buffer to be used for the (plain text) characteristic value.
	 * Must be done before init.
	 */
	cs_ret_code_t setValueBuffer(buffer_ptr_t buffer, cs_buffer_size_t size);

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

	uint16_t getValueHandle();

	uint16_t getCccdHandle();

	/**
	 * Function to be called by the stack when this characteristic is written over BLE.
	 */
	void onWrite(uint16_t len);

	/**
	 * Function to be called by the stack when the notification or indication has been sent.
	 */
	void onNotificationDone();

protected:
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
