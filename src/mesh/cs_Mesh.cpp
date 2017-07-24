/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Jan., 2015
 * License: LGPLv3+, Apache, or MIT, your choice
 */

#include <mesh/cs_Mesh.h>

// Mesh files from external repos

extern "C" {
#include <rbc_mesh.h>
#include <transport_control.h>
#include <rbc_mesh_common.h>
#include <version_handler.h>
}

#include <cfg/cs_Boards.h>

#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>

#include <util/cs_Utils.h>
#include <cfg/cs_Config.h>

#include <drivers/cs_RTC.h>

#include <storage/cs_Settings.h>
#include <storage/cs_State.h>
#include <cfg/cs_Strings.h>

#include <processing/cs_EncryptionHandler.h>

//#define PRINT_MESH_VERBOSE

//! Init message counters at 0. See: http://stackoverflow.com/questions/23987515/zero-initializing-an-array-data-member-in-a-constructor
Mesh::Mesh() :
		_appTimerData({ {0}}),
		_appTimerId(NULL),
	_started(false), _running(true), _messageCounter(), _meshControl(MeshControl::getInstance()),
	_encryptionEnabled(false)

{
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;

}

/*
const nrf_clock_lf_cfg_t Mesh::meshClockSource = {  .source        = NRF_CLOCK_LF_SRC_RC,   \
                                                    .rc_ctiv       = 16,                    \
                                                    .rc_temp_ctiv  = 2,                     \
                                                    .xtal_accuracy = 0};
*/
///*
const nrf_clock_lf_cfg_t Mesh::meshClockSource = {  .source        = NRF_CLOCK_LF_SRC_XTAL, \
                                                    .rc_ctiv       = 0,                     \
                                                    .rc_temp_ctiv  = 0,                     \
                                                    .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
//*/

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void Mesh::init() {
	_meshControl.init();

	LOGi(FMT_INIT, "Mesh");

	// setup the timer
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Mesh::staticTick);

	rbc_mesh_init_params_t init_params;

	// the access address is a configurable parameter so we get it from the settings.
	Settings::getInstance().get(CONFIG_MESH_ACCESS_ADDRESS, &init_params.access_addr);
	LOGd("Mesh access address %p", init_params.access_addr);
	init_params.enable_gatt_service = false;
	init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
	init_params.channel = MESH_CHANNEL;
#if (NORDIC_SDK_VERSION >= 11)
	init_params.lfclksrc = meshClockSource;
#else
	init_params.lfclksrc = MESH_CLOCK_SOURCE;
#endif

	uint32_t error_code;
	//! checks if softdevice is enabled etc.
	error_code = rbc_mesh_init(init_params);
	APP_ERROR_CHECK(error_code);

	for (int i = 0; i < MESH_NUM_HANDLES; ++i) {
		error_code = rbc_mesh_value_enable(i+1);
		APP_ERROR_CHECK(error_code);
	}

	_encryptionEnabled = Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED);

	_mesh_start_time = RTC::getCount();

//	error_code = rbc_mesh_value_enable(1);
//	APP_ERROR_CHECK(error_code);
//	error_code = rbc_mesh_value_enable(2);
//	APP_ERROR_CHECK(error_code);

	// do not automatically start meshing, wait for the start command
//	rbc_mesh_stop();

//	if (!Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
//		LOGi("mesh not enabled");
		rbc_mesh_stop();
		// [16.06.16] need to execute app scheduler, otherwise pstorage events will get lost ...
		// TODO: maybe need to check why that actually happens??
		app_sched_execute();
//	}

}

void start_stop_mesh(void * p_event_data, uint16_t event_size) {
	bool start = *(bool*)p_event_data;
	if (start) {
		LOGi(FMT_START, "Mesh");
		uint32_t err_code = rbc_mesh_start();
		if (err_code != NRF_SUCCESS) {
			LOGe(FMT_FAILED_TO, "start mesh", err_code);
		}
	} else {
		LOGi(FMT_STOP, "Mesh");
		uint32_t err_code = rbc_mesh_stop();
		if (err_code != NRF_SUCCESS) {
			LOGe(FMT_FAILED_TO, "stop mesh", err_code);
		}
	}
}

void Mesh::start() {
	if (!_started) {
		_started = true;
#ifdef PRINT_MESH_VERBOSE
		LOGi("start mesh");
#endif
		startTicking();
		uint32_t errorCode = app_sched_event_put(&_started, sizeof(_started), start_stop_mesh);
		APP_ERROR_CHECK(errorCode);
	}
}

void Mesh::stop() {
	if (_started) {
		_started = false;
#ifdef PRINT_MESH_VERBOSE
		LOGi("stop mesh");
#endif
		stopTicking();
		uint32_t errorCode = app_sched_event_put(&_started, sizeof(_started), start_stop_mesh);
		APP_ERROR_CHECK(errorCode);
	}
}

void Mesh::resume() {
	if (!_running) {
		_running = true;
#ifdef PRINT_MESH_VERBOSE
		LOGi("resume mesh");
#endif
		startTicking();
		uint32_t errorCode = app_sched_event_put(&_running, sizeof(_running), start_stop_mesh);
		APP_ERROR_CHECK(errorCode);
	}
}

void Mesh::pause() {
	if (_running) {
		_running = false;
#ifdef PRINT_MESH_VERBOSE
		LOGi("pause mesh");
#endif
		stopTicking();
		uint32_t errorCode = app_sched_event_put(&_running, sizeof(_running), start_stop_mesh);
		APP_ERROR_CHECK(errorCode);
	}
}

bool Mesh::isRunning() { return _running && _started; }

bool Mesh::isValidHandle(mesh_handle_t handle) {
	//! Currently, the handles are in the range: 1 to MESH_NUM_HANDLES
	return (handle > 0 && handle <= MESH_NUM_HANDLES);
}

uint16_t Mesh::getMessageCounterIndex(mesh_handle_t handle) {
	//! Currently, the handles are in the range: 1 to MESH_NUM_HANDLES
	assert(handle > 0, "Handle < 1");
	return handle - 1;
}

MeshMessageCounter& Mesh::getMessageCounter(mesh_handle_t handle) {
	return _messageCounter[getMessageCounterIndex(handle)];
}

uint32_t Mesh::getMessageCounterVal(mesh_handle_t handle) {
	if (isValidHandle(handle)) {
		return getMessageCounter(handle).getVal();
	}
	return 0;
}

void Mesh::tick() {
	checkForMessages();
	scheduleNextTick();
}

void Mesh::scheduleNextTick() {
//	LOGi("Mesh::scheduleNextTick");
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(MESH_UPDATE_FREQUENCY), this);
}

void Mesh::startTicking() {
	Timer::getInstance().start(_appTimerId, APP_TIMER_TICKS(1, APP_TIMER_PRESCALER), this);
}

void Mesh::stopTicking() {
	Timer::getInstance().stop(_appTimerId);
}


uint32_t Mesh::send(mesh_handle_t handle, void* p_data, uint16_t length) {
	LOGd("Mesh send: handle=%d length=%d, max=%d", handle, length, MAX_MESH_MESSAGE_LENGTH);
//	assert(length <= MAX_MESH_MESSAGE_LENGTH, STR_ERR_VALUE_TOO_LONG);
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		LOGe(STR_ERR_VALUE_TOO_LONG);
		return 0;
	}

	if (!_started || !Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		return 0;
	}

	if (!isValidHandle(handle)) {
		LOGe("Invalid handle: %d", handle);
		return 0;
	}

	mesh_message_t message = {};
//	memset(&message, 0, sizeof(mesh_message_t));

	message.messageCounter = (++getMessageCounter(handle)).getVal();
	memcpy(&message.payload, p_data, length);

#ifdef PRINT_MESH_VERBOSE
	LOGd("message %d:", message.messageCounter);
	BLEutil::printArray((uint8_t*)&message, sizeof(mesh_message_t));
#endif

	uint16_t messageLen = length + PAYLOAD_HEADER_SIZE;

	encrypted_mesh_message_t encryptedMessage = {};
	uint16_t encryptedLength = sizeof(encrypted_mesh_message_t);
//	memset(&encryptedMessage, 0, sizeof(encrypted_mesh_message_t));

	encodeMessage(&message, messageLen, &encryptedMessage, encryptedLength);

#ifdef PRINT_MESH_VERBOSE
	LOGd("encrypted:");
	BLEutil::printArray((uint8_t*)&encryptedMessage, sizeof(encrypted_mesh_message_t));
#endif

//	LOGd("send ch: %d, len: %d", handle, sizeof(encrypted_mesh_message_t));
	APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)&encryptedMessage, encryptedLength));
//	APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)p_data, length));

	return message.messageCounter;
}

bool Mesh::getLastMessage(mesh_handle_t handle, void* p_data, uint16_t& length) {
//	assert(length <= MAX_MESH_MESSAGE_LENGTH, STR_ERR_VALUE_TOO_LONG);

	if (!_started || !Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		LOGi("started: %s", _started ? "true" : "false");
		return false;
	}

//	LOGd("getLastMessage, ch: %d", channel);

	encrypted_mesh_message_t encryptedMessage = {};
	uint16_t encryptedLength = sizeof(encrypted_mesh_message_t);

	//! Get the last value from rbc_mesh. This copies the message to the given pointer.
	APP_ERROR_CHECK(rbc_mesh_value_get(handle, (uint8_t*)&encryptedMessage, &encryptedLength));

	if (encryptedLength != 0) {

//		LOGd("encrypted message:");
//		BLEutil::printArray(&encryptedMessage, length);

		mesh_message_t message = {};

		bool validated = decodeMessage(&encryptedMessage, encryptedLength, &message, sizeof(mesh_message_t));
		if (!validated) {
			LOGe("Message not validated!");
			return false;
		}

//		LOGd("message:");
//		BLEutil::printArray(&message, length);

		if (length > sizeof(message.payload)) {
			length = sizeof(message.payload);
		}
		memcpy((uint8_t*)p_data, message.payload, length);

		//LOGi("recv ch: %d, len: %d", handle, length);
		return true;

	} else {
		return false;
	}
}

void Mesh::resolveConflict(mesh_handle_t handle, encrypted_mesh_message_t* p_old, uint16_t length_old,
		encrypted_mesh_message_t* p_new, uint16_t length_new)
{

	mesh_message_t messageOld, messageNew;

	bool validatedOld = decodeMessage(p_old, length_old, &messageOld, sizeof(mesh_message_t));
	bool validatedNew = decodeMessage(p_new, length_new, &messageNew, sizeof(mesh_message_t));
	if (!validatedOld && validatedNew) {
		LOGw("Old msg not validated");
		// TODO: should we send a new message? All crownstones with same key should ignore the invalid one.
//		getMessageCounter(handle).setVal(messageNew.messageCounter);
//		send(handle, messageNew.payload, sizeof(messageNew.payload));
//		_meshControl.process(handle, &messageNew, sizeof(mesh_message_t));
		return;
	}
	else if (validatedOld && !validatedNew) {
		LOGw("New msg not validated");
		return;
	}
	else if (!validatedOld && !validatedNew) {
		LOGw("Both msgs not validated");
		return;
	}

	switch(handle) {
	case COMMAND_REPLY_CHANNEL: {

		reply_message_t *replyMessageOld, *replyMessageNew;
		replyMessageOld = (reply_message_t*)messageOld.payload;
		replyMessageNew = (reply_message_t*)messageNew.payload;

		//! Check if the reply msgs are valid (check numOfReplys field)
		//! TODO: do we have to check the msg length here? Think not, since mesh messages are always full size.
		if (!is_valid_reply_msg(replyMessageOld) || !is_valid_reply_msg(replyMessageNew)) {
			return;
		}

#ifdef PRINT_MESH_VERBOSE
		LOGi("replyMessageOld:");
		BLEutil::printArray(replyMessageOld, sizeof(reply_message_t));
		LOGi("replyMessageNew:");
		BLEutil::printArray(replyMessageNew, sizeof(reply_message_t));
#endif

		//! The message counter is used to identify the to which command this message replies to.
		int32_t delta = MeshMessageCounter::calcDelta(replyMessageOld->messageCounter, replyMessageNew->messageCounter);
		if (delta > 0) {

			LOGi("new is newer command reply");

			//! Update message counter
			getMessageCounter(handle).setVal(messageNew.messageCounter);

			//! Send resolved message into mesh
			send(handle, replyMessageNew, sizeof(reply_message_t));

			//! and process the new message
			_meshControl.process(handle, &messageNew, sizeof(mesh_message_t));
		} else if (delta < 0) {

			LOGi("old is newer command reply");

			//! Update message counter
			getMessageCounter(handle).setVal(messageOld.messageCounter);

			//! Send resolved message into mesh
			send(handle, replyMessageOld, sizeof(reply_message_t));

			//! and process the old message
			_meshControl.process(handle, &messageOld, sizeof(mesh_message_t));
		} else {

			//! Replies should not be of a different type
			if (replyMessageNew->messageType != replyMessageOld->messageType) {
				LOGe("ohoh, different message types");
				return;
			}

			if (replyMessageNew->messageType == STATUS_REPLY) {

				//! Merge by pushing all items from the new message that are not in the old message to the old message.

				//! Loop over all items in new message.
				status_reply_item_t* srcItem;
				for (int i = 0; i < replyMessageNew->numOfReplys; ++i) {
					srcItem = &replyMessageNew->statusList[i];

					//! Check if item is in old message.
					status_reply_item_t* destItem;
					bool found = false;
					for (int j = 0; j < replyMessageOld->numOfReplys; ++j) {
						destItem = &replyMessageOld->statusList[j];
						if (destItem->id == srcItem->id) {
							found = true;
							break;
						}
					}

					//! If item is not in old message, then push it to the old message.
					if (!found) {
						push_status_reply_item(replyMessageOld, srcItem);
					}

				}

#ifdef PRINT_MESH_VERBOSE
				LOGi("merged:");
				BLEutil::printArray(replyMessageOld, sizeof(reply_message_t));
#endif

				//! update message counter
				getMessageCounter(handle).setVal(messageNew.messageCounter);

				//! send resolved message into mesh
				send(handle, replyMessageOld, sizeof(reply_message_t));

				//! and process
				_meshControl.process(handle, &messageOld, sizeof(mesh_message_t));

			}
			//! TODO: handle conflicts for state and config replies?
		}


		break;
	}
	case STATE_BROADCAST_CHANNEL:
	case STATE_CHANGE_CHANNEL: {

		state_message_t *stateMessageOld, *stateMessageNew;
		stateMessageOld = (state_message_t*)messageOld.payload;
		stateMessageNew = (state_message_t*)messageNew.payload;

		//! Check if the state msgs are valid (check head, tail, size fields)
		if (!is_valid_state_msg(stateMessageOld) || !is_valid_state_msg(stateMessageNew)) {
			return;
		}

#ifdef PRINT_MESH_VERBOSE
		LOGi("stateMessageOld:");
		BLEutil::printArray(stateMessageOld, sizeof(state_message_t));
		LOGi("stateMessageNew:");
		BLEutil::printArray(stateMessageNew, sizeof(state_message_t));
#endif

		//! Merge by pushing all items from the new message that are not in the old message to the old message.

		//! Loop new message from oldest to newest item, so that the newest item is push lastly.
		int16_t srcIndex = -1;
		state_item_t* srcItem;
		while (peek_next_state_item(stateMessageNew, &srcItem, srcIndex)) {
			//! Check if item is in the old message.
			bool found = false;
			int16_t destIndex = -1;
			state_item_t* destItem;
			while (peek_next_state_item(stateMessageOld, &destItem, destIndex)) {
				if (memcmp(destItem, srcItem, sizeof(state_item_t)) == 0) {
					found = true;
					break;
				}
			}

			//! If item is not in old message, then push it to the old message.
			if (!found) {
				push_state_item(stateMessageOld, srcItem);
			}
		}

		//! Set the timestamp to the current time
		uint32_t timestamp;
		if (State::getInstance().get(STATE_TIME, timestamp) != ERR_SUCCESS) {
			timestamp = 0;
		}
		stateMessageOld->timestamp = timestamp;

#ifdef PRINT_MESH_VERBOSE
		LOGi("merged:");
		BLEutil::printArray(stateMessageOld, sizeof(state_message_t));
#endif

		//! update message counter
		getMessageCounter(handle).setVal(messageNew.messageCounter);

		//! send resolved message into mesh
		send(handle, stateMessageOld, sizeof(state_message_t));

		//! and process
		_meshControl.process(handle, &messageOld, sizeof(messageOld));

		break;
	}
	}
}

void Mesh::handleMeshMessage(rbc_mesh_event_t* evt)
{
	mesh_handle_t handle;
	encrypted_mesh_message_t* received;
	uint16_t receivedLength;

	TICK_PIN(28);

	handle = evt->params.rx.value_handle;
	received = (encrypted_mesh_message_t*)evt->params.rx.p_data;
	receivedLength = evt->params.rx.data_len;

#ifdef PRINT_MESH_VERBOSE
	switch (evt->type)
	{
	case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL: {
		LOGd("ch: %d, conflicting value", handle);
		break;
	}
	case RBC_MESH_EVENT_TYPE_NEW_VAL:
		LOGd("ch: %d, new value", handle);
		break;
	case RBC_MESH_EVENT_TYPE_UPDATE_VAL:
//		LOGd("ch: %d, update value", handle);
		break;
	case RBC_MESH_EVENT_TYPE_INITIALIZED:
		LOGd("ch: %d, initialized", handle);
		break;
	case RBC_MESH_EVENT_TYPE_TX:
		LOGd("ch: %d, transmitted", handle);
		break;
	default:
		LOGd("ch: %d, evt: %d", handle, evt->type);
	}
#endif

	//! Check if the handle is valid.
	if (!isValidHandle(handle)) {
		uint32_t error_code;
		error_code = rbc_mesh_value_disable(handle);
		APP_ERROR_CHECK(error_code);
		return;
	}

	switch (evt->type)
	{
		case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL: {

			encrypted_mesh_message_t stored;
			uint16_t storedLength = sizeof(encrypted_mesh_message_t);
			APP_ERROR_CHECK(rbc_mesh_value_get(handle, (uint8_t*)&stored, &storedLength));

			int32_t delta = MeshMessageCounter::calcDelta(stored.messageCounter, received->messageCounter);
			if (delta >= 0) {

//				LOGd("received data:");
//				BLEutil::printArray(received, receivedLength);
//				LOGd("stored data:");
//				BLEutil::printArray(&stored, storedLength);

//				if (delta < 0) { // This seems impossible?
//					resolveConflict(handle, received, receivedLength, &stored, storedLength);
//				} else {
				resolveConflict(handle, &stored, storedLength, received, receivedLength);
//				}
			}
			break;
		}
		case RBC_MESH_EVENT_TYPE_NEW_VAL:
		case RBC_MESH_EVENT_TYPE_UPDATE_VAL: {

//			LOGd("MESH RECEIVE");

			//! Decrypt to a newly allocated message, so that we don't change the mesh handle value.
			mesh_message_t message;

#ifdef PRINT_MESH_VERBOSE
            LOGi("Got data handle: %d, len: %d", handle, receivedLength);
            BLEutil::printArray(received, receivedLength);
#endif

			if (decodeMessage(received, receivedLength, &message, sizeof(mesh_message_t))) {

				int32_t delta = getMessageCounter(handle).calcDelta(received->messageCounter);
				if (delta > 0) {

					getMessageCounter(handle).setVal(message.messageCounter);

#ifdef PRINT_MESH_VERBOSE
					LOGi("message:");
					BLEutil::printArray(&message, sizeof(mesh_message_t));
#endif

					_meshControl.process(handle, &message, sizeof(mesh_message_t));

				}
				else {
					LOGw("Received older msg? received %d, stored %d", message.messageCounter, getMessageCounter(handle).getVal());
				}
			}
            break;
        }
        default:
            break;
	}
}

bool Mesh::decodeMessage(encrypted_mesh_message_t* encoded, uint16_t encodedLength, mesh_message_t* decoded, uint16_t decodedLength) {

	if (_encryptionEnabled) {
		return EncryptionHandler::getInstance().decryptMesh((uint8_t*)encoded, sizeof(encrypted_mesh_message_t),
				&decoded->messageCounter, decoded->payload, sizeof(decoded->payload));
	} else {
		memcpy(decoded, encoded->encrypted_payload, decodedLength);
		return encoded->messageCounter == decoded->messageCounter;
	}
	return true;
}

bool Mesh::encodeMessage(mesh_message_t* decoded, uint16_t decodedLength, encrypted_mesh_message_t* encoded, uint16_t encodedLength) {

	if (_encryptionEnabled) {
		return EncryptionHandler::getInstance().encryptMesh(decoded->messageCounter, decoded->payload, sizeof(decoded->payload),
				(uint8_t*)encoded, sizeof(encrypted_mesh_message_t));
	} else {
		memcpy(encoded->encrypted_payload, decoded, decodedLength);
		encoded->messageCounter = decoded->messageCounter;
		return true;
	}
}

void Mesh::checkForMessages() {
	rbc_mesh_event_t evt;

	//! check if there are new messages
	while (rbc_mesh_event_get(&evt) == NRF_SUCCESS) {

		mesh_handle_t handle = evt.params.tx.value_handle;

		//! Check if the handle is valid.
		if (isValidHandle(handle)) {

			encrypted_mesh_message_t* received = (encrypted_mesh_message_t*)evt.params.rx.p_data;
//			if (getMessageCounter(handle).getVal() == 0) {
//				//! Skip handling the first message we receive on each handle, as it probably is an old message.
			if (getMessageCounter(handle).getVal() == 0 && RTC::difference(RTC::getCount(), _mesh_start_time) < RTC::msToTicks(MESH_BOOT_TIME)) {
				//! Skip handling the first message we receive within the first X seconds after boot on each handle, as it probably is an old message.
				//! This may lead to ignoring a message that should've been handled.
				LOGi("skip message %d on handle %d", received->messageCounter, handle);
				//! Skip the first message after a boot up but take over the message counter
				getMessageCounter(handle).setVal(received->messageCounter);
			}
			else {
				//! handle the message
				handleMeshMessage(&evt);
			}
		}
		else {
			//! then free associated memory
			//! TODO: why do we have release the event pointer if we allocated the memory ourselves?
			rbc_mesh_event_release(&evt);

			uint32_t error_code;
			error_code = rbc_mesh_value_disable(handle);
			APP_ERROR_CHECK(error_code);
		}

		//! then free associated memory
		//! TODO: why do we have release the event pointer if we allocated the memory ourselves?
		rbc_mesh_event_release(&evt);
	}
}

