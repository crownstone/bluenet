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
#include <cfg/cs_Strings.h>

#include <processing/cs_EncryptionHandler.h>

//#define PRINT_MESH_VERBOSE

Mesh::Mesh() :
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerData({ {0}}),
		_appTimerId(NULL),
#else
		_appTimerId(UINT32_MAX),
#endif
	_started(false), _running(true), _messageCounter(), _meshControl(MeshControl::getInstance()),
	_encryptionEnabled(false)

{
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;
#endif

}

#if (NORDIC_SDK_VERSION >= 11)
const nrf_clock_lf_cfg_t Mesh::meshClockSource = {  .source        = NRF_CLOCK_LF_SRC_RC,   \
                                                    .rc_ctiv       = 16,                    \
                                                    .rc_temp_ctiv  = 2,                     \
                                                    .xtal_accuracy = 0};
#endif

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void Mesh::init() {
	_meshControl.init();

	//nrf_gpio_pin_clear(PIN_GPIO_LED0);
	LOGi(FMT_INIT, "Mesh");

	// setup the timer
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Mesh::staticTick);

	rbc_mesh_init_params_t init_params;

	// the access address is a configurable parameter so we get it from the settings.
	Settings::getInstance().get(CONFIG_MESH_ACCESS_ADDRESS, &init_params.access_addr);
	LOGd("Mesh access address %p", init_params.access_addr);
	init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
	init_params.channel = MESH_CHANNEL;
#if (NORDIC_SDK_VERSION >= 11)
	init_params.lfclksrc = meshClockSource;
#else
	init_params.lfclksrc = MESH_CLOCK_SOURCE;
#endif

	uint8_t error_code;
	//! checks if softdevice is enabled etc.
	error_code = rbc_mesh_init(init_params);
	APP_ERROR_CHECK(error_code);

	for (int i = 0; i < MESH_NUM_HANDLES; ++i) {
		error_code = rbc_mesh_value_enable(i+1);
		APP_ERROR_CHECK(error_code);
	}

//	error_code = rbc_mesh_value_enable(1);
//	APP_ERROR_CHECK(error_code);
//	error_code = rbc_mesh_value_enable(2);
//	APP_ERROR_CHECK(error_code);

	// do not automatically start meshing, wait for the start command
//	rbc_mesh_stop();

//	if (!Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
//		LOGi("mesh not enabled");
		rbc_mesh_stop();
		// [16.06.16] need to execute app scheduler, otherwise pstorage
		// events will get lost ... maybe need to check why that actually happens??
		app_sched_execute();
//	}

}

void start_stop_mesh(void * p_event_data, uint16_t event_size) {
	if (*(bool*)p_event_data) {
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
		app_sched_event_put(&_started, sizeof(_started), start_stop_mesh);
	}
}

void Mesh::stop() {
	if (_started) {
		_started = false;
#ifdef PRINT_MESH_VERBOSE
		LOGi("stop mesh");
#endif
		stopTicking();
		app_sched_event_put(&_started, sizeof(_started), start_stop_mesh);
	}
}

void Mesh::resume() {
	if (!_running) {
		_running = true;
#ifdef PRINT_MESH_VERBOSE
		LOGi("resume mesh");
#endif
		startTicking();
		app_sched_event_put(&_running, sizeof(_running), start_stop_mesh);
	}
}

void Mesh::pause() {
	if (_running) {
		_running = false;
#ifdef PRINT_MESH_VERBOSE
		LOGi("pause mesh");
#endif
		stopTicking();
		app_sched_event_put(&_running, sizeof(_running), start_stop_mesh);
	}
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


uint32_t Mesh::send(uint8_t handle, void* p_data, uint8_t length) {
//	LOGe("length: %d, max: %d", length, MAX_MESH_MESSAGE_LENGTH);
//	assert(length <= MAX_MESH_MESSAGE_LENGTH, STR_ERR_VALUE_TOO_LONG);
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		LOGe(STR_ERR_VALUE_TOO_LONG);
		return 0;
	}

	if (!_started || !Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		return 0;
	}

	mesh_message_t message = {};
//	memset(&message, 0, sizeof(mesh_message_t));

	message.messageCounter = ++_messageCounter[handle];
	memcpy(&message.payload, p_data, length);

#ifdef PRINT_MESH_VERBOSE
	LOGd("message:");
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
//	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)&encryptedMessage, encryptedLength));
//		APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)p_data, length));
//	}

	return message.messageCounter;
}

bool Mesh::getLastMessage(uint8_t channel, void* p_data, uint16_t& length) {
//	assert(length <= MAX_MESH_MESSAGE_LENGTH, STR_ERR_VALUE_TOO_LONG);

	if (!_started || !Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		LOGi("started: %s", _started ? "true" : "false");
		return false;
	}

//	LOGd("getLastMessage, ch: %d", channel);

	encrypted_mesh_message_t encryptedMessage = {};
	uint16_t encryptedLength = sizeof(encrypted_mesh_message_t);

//	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		APP_ERROR_CHECK(rbc_mesh_value_get(channel, (uint8_t*)&encryptedMessage, &encryptedLength));
//	}

	if (encryptedLength != 0) {

//		LOGd("encrypted message:");
//		BLEutil::printArray(&encryptedMessage, length);

		mesh_message_t message = {};

		decodeMessage(&encryptedMessage, encryptedLength, &message, sizeof(mesh_message_t));

//		LOGd("message:");
//		BLEutil::printArray(&message, length);

		length = sizeof(message.payload);
		memcpy((uint8_t*)p_data, message.payload, length);

		//LOGi("recv ch: %d, len: %d", handle, length);
		return true;

	} else {
		return false;
	}
}

void Mesh::resolveConflict(uint8_t handle, encrypted_mesh_message_t* p_old, uint16_t length_old,
		encrypted_mesh_message_t* p_new, uint16_t length_new)
{

	mesh_message_t messageOld, messageNew;

//	LOGd("old data:");
//	BLEutil::printArray(p_old, length_old);
//	LOGd("new data:");
//	BLEutil::printArray(p_new, length_new);

	decodeMessage(p_old, length_old, &messageOld, sizeof(mesh_message_t));
	decodeMessage(p_new, length_new, &messageNew, sizeof(mesh_message_t));

	switch(handle) {
	case COMMAND_REPLY_CHANNEL: {

		reply_message_t *replyMessageOld, *replyMessageNew;

		replyMessageOld = (reply_message_t*)messageOld.payload;

//		LOGi("replyMessageOld:");
//		BLEutil::printArray(replyMessageOld, sizeof(reply_message_t));

		replyMessageNew = (reply_message_t*)messageNew.payload;

//		LOGi("replyMessageNew:");
//		BLEutil::printArray(replyMessageNew, sizeof(reply_message_t));

		if (replyMessageNew->messageCounter > replyMessageOld->messageCounter) {

			LOGi("new is newer command reply");

			// update message counter
			_messageCounter[handle] = messageNew.messageCounter;

			// send resolved message into mesh
			send(handle, replyMessageNew, sizeof(reply_message_t));

			// and process
	//		_meshControl.process(handle, stateMessageOld, sizeof(state_message_t));
			_meshControl.process(handle, &messageNew, sizeof(mesh_message_t));
		} else if (replyMessageOld->messageCounter > replyMessageNew->messageCounter) {

			LOGi("old is newer command reply");

			// update message counter
			_messageCounter[handle] = messageOld.messageCounter;

			// send resolved message into mesh
			send(handle, replyMessageOld, sizeof(reply_message_t));

			// and process
	//		_meshControl.process(handle, stateMessageOld, sizeof(state_message_t));
			_meshControl.process(handle, &messageOld, sizeof(mesh_message_t));
		} else {

//			LOGi("merge ...");

			if (replyMessageNew->messageType != replyMessageOld->messageType) {
				LOGe("ohoh, different message types");
				return;
			}

			if (replyMessageNew->messageType == STATUS_REPLY) {


				status_reply_item_t* srcItem;
				for (int i = 0; i < replyMessageNew->numOfReplys; ++i) {

					srcItem = &replyMessageNew->statusList[i];

					status_reply_item_t* destItem;
					bool found = false;
					for (int j = 0; j < replyMessageOld->numOfReplys; ++j) {
						destItem = &replyMessageOld->statusList[j];

						if (destItem->id == srcItem->id) {
//							LOGi("index %d found at %d", i, j);
//							BLEutil::printArray(srcItem, sizeof(status_reply_item_t));

							found = true;
							break;
						}
					}

					if (!found) {
//						LOGi("index %d not found, push back:", i);
//						BLEutil::printArray(srcItem, sizeof(status_reply_item_t));

						push_status_reply_item(replyMessageOld, srcItem);
					}

				}

//				LOGi("update final");
//				BLEutil::printArray(replyMessageOld, sizeof(reply_message_t));

				//! update message counter
				_messageCounter[handle] = messageNew.messageCounter;

				//! send resolved message into mesh
				send(handle, replyMessageOld, sizeof(reply_message_t));

				//! and process
		//		_meshControl.process(handle, stateMessageOld, sizeof(state_message_t));
				_meshControl.process(handle, &messageOld, sizeof(mesh_message_t));

			}
		}


		break;
	}
	case STATE_BROADCAST_CHANNEL:
	case STATE_CHANGE_CHANNEL: {

		state_message_t *stateMessageOld, *stateMessageNew;

		stateMessageOld = (state_message_t*)messageOld.payload;

//		LOGi("stateMessageOld:");
//		BLEutil::printArray(stateMessageOld, sizeof(state_message_t));

		stateMessageNew = (state_message_t*)messageNew.payload;

//		LOGi("stateMessageNew:");
//		BLEutil::printArray(stateMessageNew, sizeof(state_message_t));

		if (stateMessageOld->head > stateMessageNew->head) {
//			LOGi("update new head");
			stateMessageNew->head = stateMessageOld->head;
		} else if (stateMessageNew->head > stateMessageOld->head) {
			stateMessageOld->head = stateMessageNew->head;
//			LOGi("update old head");
		}

//			LOGi("merge ...");

		int16_t srcIndex = -1;
		state_item_t* srcItem;
		while (peek_next_state_item(stateMessageNew, &srcItem, srcIndex)) {

			bool found = false;

			int16_t destIndex = -1;
			state_item_t* destItem;
			while (peek_next_state_item(stateMessageOld, &destItem, destIndex)) {

				if (memcmp(destItem, srcItem, sizeof(state_item_t)) == 0) {

//						LOGi("index %d found already:", srcIndex);
//						BLEutil::printArray(srcItem, sizeof(state_item_t));

					found = true;
					break;
				}

			}

			if (!found) {
				push_state_item(stateMessageOld, srcItem);

//					LOGi("index %d not found, push back:", srcIndex);
//					BLEutil::printArray(srcItem, sizeof(state_item_t));
			}
		}

//		LOGi("update final");
//		BLEutil::printArray(stateMessageOld, sizeof(state_message_t));

		//! update message counter
		_messageCounter[handle] = messageNew.messageCounter;

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
	rbc_mesh_value_handle_t handle;
	encrypted_mesh_message_t* received;
	uint16_t receivedLength;

	TICK_PIN(28);
//	nrf_gpio_gitpin_toggle(PIN_GPIO_LED1);

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

	if (handle > MESH_NUM_HANDLES) {
		rbc_mesh_value_disable(handle);
		return;
	}

	switch (evt->type)
	{
		case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL: {

			encrypted_mesh_message_t stored;
			uint16_t storedLength = sizeof(encrypted_mesh_message_t);
			APP_ERROR_CHECK(rbc_mesh_value_get(handle, (uint8_t*)&stored, &storedLength));

			if (received->messageCounter >= stored.messageCounter) {

//				LOGd("received data:");
//				BLEutil::printArray(received, receivedLength);
//				LOGd("stored data:");
//				BLEutil::printArray(&stored, storedLength);

				if (stored.messageCounter > received->messageCounter) {
					resolveConflict(handle, received, receivedLength, &stored, storedLength);
				} else {
					resolveConflict(handle, &stored, storedLength, received, receivedLength);
				}
			}
			break;
		}
		case RBC_MESH_EVENT_TYPE_NEW_VAL:
		case RBC_MESH_EVENT_TYPE_UPDATE_VAL: {

//			LOGd("MESH RECEIVE");

			mesh_message_t message;

#ifdef PRINT_MESH_VERBOSE
            LOGi("Got data handle: %d, len: %d", handle, receivedLength);
            BLEutil::printArray(received, receivedLength);
#endif

			if (decodeMessage(received, receivedLength, &message, sizeof(mesh_message_t))) {

//				if (message.messageCounter > _messageCounter[handle]) {

					_messageCounter[handle] = message.messageCounter;

#ifdef PRINT_MESH_VERBOSE
					LOGi("message:");
					BLEutil::printArray(&message, sizeof(mesh_message_t));
#endif

					_meshControl.process(handle, &message, sizeof(mesh_message_t));

//				}
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
		return true;
	}
}

void Mesh::checkForMessages() {
	rbc_mesh_event_t evt;

	//! check if there are new messages
	while (rbc_mesh_event_get(&evt) == NRF_SUCCESS) {

		uint8_t handle = evt.params.tx.value_handle;
		if (_messageCounter[handle] == 0) {
			encrypted_mesh_message_t* received = (encrypted_mesh_message_t*)evt.params.rx.p_data;

#ifdef NDEBUG
			_messageCounter[handle] = received->messageCounter;
			LOGi("skip message %d on handle %d", received->messageCounter, handle);
			continue;
#else
			if (received->messageCounter == 1) {
				//! ok
			} else {
				//! skip the first message after a boot up but take over the message counter
				_messageCounter[handle] = received->messageCounter;
				LOGi("skip message %d on handle %d", received->messageCounter, handle);
				continue;
			}
#endif
		}

		//! handle the message
		handleMeshMessage(&evt);

		//! then free associated memory
		rbc_mesh_event_release(&evt);
	}
}

