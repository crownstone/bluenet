/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <protocol/cs_MeshMessageTypes.h>
#include <storage/cs_Settings.h>

/** Wrapper around meshing functionality.
 *
 */
class MeshControl : public EventListener {

public:
	//! use static variant of singleton, no dynamic memory allocation
	static MeshControl& getInstance() {
		static MeshControl instance;
		return instance;
	}

	/** Send a message with the scanned devices over the mesh
	 *
	 * @p_list pointer to the list of scanned devices
	 * @size number of scanned devices
	 */
	void sendScanMessage(peripheral_device_t* p_list, uint8_t size);

//	void sendPowerSamplesMessage(power_samples_mesh_message_t* samples);

	/** Send a service data message (state broadcast or state change) over the mesh
	 *
	 * @stateItem the state item to be appended to the message in the mesh
	 * @event true if the state should be sent as a state change
	 *        false if the state should be sent as a state broadcast
	 */
	void sendServiceDataMessage(state_item_t& stateItem, bool event);

	/** Send a status reply message, e.g. to reply status code of the execution of a command
	 *
	 * @messageCounter the counter of the command message to which this reply belongs
	 * @status the status code of the command execution
	 */
	void sendStatusReplyMessage(uint32_t messageCounter, ERR_CODE status);

	/** Send a config reply over the mesh (for read/get config commands)
	 *
	 * @messageCounter the counter of the read/get config command message
	 * @configReply the config to be sent
	 */
	void sendConfigReplyMessage(uint32_t messageCounter, config_reply_item_t* configReply);

	/** Send a state variable reply over the mesh (for read state variable commands)
	 *
	 * @messageCounter the counter of the read/get state variable command message
	 * @stateReply the state variable to be sent
	 */
	void sendStateReplyMessage(uint32_t messageCounter, state_reply_item_t* stateReply);

	/** Send a message into the mesh
	 *
	 * @channel the channel number, see <MeshChannels>
	 * @p_data a pointer to the data which should be sent
	 * @length number of bytes of data to with p_data points
	 */
	ERR_CODE send(uint8_t channel, void* p_data, uint8_t length);

	/**
	 * Get incoming messages and perform certain actions.
	 * @channel the channel number, see <MeshChannels>
	 *
	 * @p_data a pointer to the data which was received
	 * @length number of bytes received
	 */
	void process(uint8_t channel, void* p_data, uint16_t length);

	/** Initialize the mesh */
	void init();

protected:

	/** Handle events dispatched through the EventDispatcher
	 *
	 * @evt the event type, can be one of <GeneralEventType>, <ConfigurationTypes>, <StateVarTypes>
	 * @p_data pointer to the data which was sent with the dispatch, can be NULL
	 * @length number of bytes of data provided
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	/** Decode a command message received over mesh or mesh characteristic
	 *
	 * @type command type, see MeshCommandTypes
	 * @messageCounter the message counter of the message which was received
	 * @payload the payload of the command
	 * @length length of payload in bytes
	 */
	void handleCommand(uint16_t type, uint32_t messageCounter, uint8_t* payload, uint16_t length);

	/** Decode a keep alive message received over mesh or mesh characteristic
	 *
	 * @msg the message payload
	 * @length length of the message in bytes
	 */
	ERR_CODE handleKeepAlive(keep_alive_message_t* msg, uint16_t length);

	/** Decode a multi switch message received over mesh or mesh characteristic
	 *
	 * @msg the message payload
	 * @length length of the message in bytes
	 */
	ERR_CODE handleMultiSwitch(multi_switch_message_t* msg, uint16_t length);

private:
	MeshControl();

	MeshControl(MeshControl const&); //! singleton, deny implementation
	void operator=(MeshControl const &); //! singleton, deny implementation

	/* The id assigned to this Crownstone during setup */
    id_type_t _myCrownstoneId;

};
