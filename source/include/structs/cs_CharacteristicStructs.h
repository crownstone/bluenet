/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 1, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_Packets.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <cstdint>

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

	//! The connected device subscribed or unsubscribed for notifications or indications.
	CHARACTERISTIC_EVENT_SUBSCRIPTION,

	//! The notification of indication was sent.
	CHARACTERISTIC_EVENT_NOTIFY_DONE,
};
