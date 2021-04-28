/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include <events/cs_EventListener.h>
#include <structs/cs_PacketsInternal.h>
#include <structs/cs_ControlPacketAccessor.h>
#include <structs/cs_ResultPacketAccessor.h>

/** General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class CrownstoneService: public Service, EventListener {
public:
	/** Constructor for general crownstone service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store name and other information that is
	 * set over so-called configuration characteristics. It also initializes all characteristics.
	 */
	 CrownstoneService();

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be
	 * resolved immediately in interrupt service handlers. The temperature for example is updated every
	 * tick, because timing is not important for this at all.
	 */
//	void tick();

//	void scheduleNextTick();

	virtual void handleEvent(event_t & event);

protected:
	/** Initialize a CrownstoneService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

	/** Enable the control characteristic.
 	 */
	void addControlCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size, uint16_t charUuid,
			EncryptionAccessLevel minimumAccessLevel);

	/**
	 * Enable the result characteristic.
	 */
	void addResultCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size, uint16_t charUuid,
			EncryptionAccessLevel minimumAccessLevel);

	void addFactoryResetCharacteristic();

	void addSessionDataCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size,
			EncryptionAccessLevel minimumAccessLevel = BASIC);

	void getReadBuffer(buffer_ptr_t& buffer, cs_buffer_size_t& maxLength);
	void getWriteBuffer(buffer_ptr_t& buffer, cs_buffer_size_t& maxLength);

	void removeBuffer();

protected:
	Characteristic<buffer_ptr_t>* _controlCharacteristic = nullptr;
	Characteristic<buffer_ptr_t>* _resultCharacteristic = nullptr;

	ControlPacketAccessor<>* _controlPacketAccessor = nullptr;
	ResultPacketAccessor<>* _resultPacketAccessor = nullptr;

	/** Write a result to the result characteristic.
	 *
	 * @param[in] protocol        The protocol version.
	 * @param[in] type            The command type that was handled.
	 * @param[in] result          The result of handling the command.
	 */
	void writeResult(uint8_t protocol, CommandHandlerTypes type, cs_result_t & result);

	/** Write a result to the result characteristic.
	 *
	 * @param[in] protocol        The protocol version.
	 * @param[in] type            The command type that was handled.
	 * @param[in] retCode         The result code of handling the command.
	 * @param[in] data            The result data of handling the command.
	 */
	void writeResult(uint8_t protocol, CommandHandlerTypes type, cs_ret_code_t retCode, cs_data_t data);


private:
	uint8_t _keySessionDataBuffer[sizeof(session_data_t)];

	Characteristic<buffer_ptr_t>* _sessionDataCharacteristic = nullptr;
	Characteristic<buffer_ptr_t>* _sessionDataUnencryptedCharacteristic = nullptr;
	Characteristic<uint32_t>*     _factoryResetCharacteristic = nullptr;
};
