/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <cfg/cs_AutoConfig.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Twi.h>
#include <events/cs_EventDispatcher.h>

#define TWI_INSTANCE_ID 0

const nrfx_twi_t Twi::_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

void twiEventHandler(nrfx_twi_evt_t const* event, void* context) {
	switch (event->type) {
		case NRFX_TWI_EVT_DONE:
			if (event->xfer_desc.type == NRFX_TWI_XFER_RX) {
				Twi::getInstance().isrEvent(TwiIsrEvent::Read);
			}
			else {
				Twi::getInstance().isrEvent(TwiIsrEvent::Write);
			}
			break;
		case NRFX_TWI_EVT_ADDRESS_NACK:
			LOGw("TWI address nack (check pins/connection and the address)");
			Twi::getInstance().isrEvent(TwiIsrEvent::Error);
			break;
		case NRFX_TWI_EVT_DATA_NACK:
			LOGw("TWI data nack");
			Twi::getInstance().isrEvent(TwiIsrEvent::Error);
			break;
		// case NRFX_TWI_EVT_OVERRUN:
		//	LOGw("TWI overrun");
		//	break;
		default:
			LOGw("Unknown event");
			Twi::getInstance().isrEvent(TwiIsrEvent::Error);
			break;
	}
}

Twi& Twi::getInstance() {
	static Twi instance;
	return instance;
}

Twi::Twi() : EventListener(), _initialized(false), _initializedBus(false) {
	EventDispatcher::getInstance().addListener(this);

	_eventRead  = false;
	_eventError = false;
}

void Twi::isrEvent(TwiIsrEvent event) {
	switch (event) {
		case TwiIsrEvent::Read:
			_eventRead  = true;
			_eventError = false;
			break;
		case TwiIsrEvent::Write: break;
		case TwiIsrEvent::Error: _eventError = true; break;
	}
}

/*
 * General initialization from board config.
 */
void Twi::init(const boards_config_t& board) {
	_config.sda  = board.pinGpio[g_TWI_SDA_INDEX];
	_config.scl  = board.pinGpio[g_TWI_SCL_INDEX];
	_initialized = true;
	LOGi("Configured TWI at SDA %i and SCL %i", _config.sda, _config.scl);
}

/*
 * Only operate as master.
 */
void Twi::initBus(cs_twi_init_t& twi) {
	LOGi("Init TWI bus");
	if (!_initialized) {
		LOGw("Class not initialized.");
		return;
	}
	if (_initializedBus) {
		LOGw("Already initialized bus.");
		return;
	}
	_config.frequency          = NRF_TWI_FREQ_100K;
	_config.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;
	_config.hold_bus_uninit    = false;
	nrfx_err_t ret             = nrfx_twi_init(&_twi, &_config, twiEventHandler, NULL);
	if (ret != ERR_SUCCESS) {
		LOGw("Init error: %i", ret);
		return;
	}
	nrfx_twi_enable(&_twi);

	_initializedBus = true;
}

void Twi::write(uint8_t address, uint8_t* data, size_t length, bool stop) {
	if (!_initialized || !_initializedBus) {
		LOGw("Twi not initialized yet, cannot write");
		return;
	}
	if (length < 1) {
		LOGw("Nothing to write");
		return;
	}
	LOGd("Write i2c value starting with [0x%x,...]", data[0]);

	nrfx_twi_tx(&_twi, address, data, length, !stop);
}

void Twi::read(uint8_t address, uint8_t* data, size_t& length) {
	if (!_initialized || !_initializedBus) {
		LOGw("Twi not initialized yet, cannot read");
		length = 0;
		return;
	}
	uint16_t ret_code;
	// LOGi("Read i2c value");

	nrfx_twi_xfer_desc_t xfer;
	xfer.type            = NRFX_TWI_XFER_RX;
	xfer.address         = address;
	xfer.primary_length  = length;
	xfer.p_primary_buf   = data;
	const uint32_t flags = 0;

	while (true) {
		_eventRead  = false;
		_eventError = false;
		ret_code    = nrfx_twi_xfer(&_twi, &xfer, flags);
		if (ret_code != NRFX_SUCCESS) {
			length = 0;
			switch (ret_code) {
				case NRFX_ERROR_BUSY: LOGd("Busy error with twi rx: %x", ret_code); break;
				case NRFX_ERROR_INTERNAL: LOGw("Internal error with twi rx: %x", ret_code); break;
				case NRFX_ERROR_INVALID_STATE: LOGw("State error with twi rx: %x", ret_code); break;
				case NRFX_ERROR_DRV_TWI_ERR_OVERRUN: LOGw("Overrun error with twi rx: %x", ret_code); break;
				case NRFX_ERROR_DRV_TWI_ERR_ANACK: LOGw("Anack error with twi rx: %x", ret_code); break;
				case NRFX_ERROR_DRV_TWI_ERR_DNACK: LOGw("Dnack error with twi rx: %x", ret_code); break;
				default: LOGw("Unkown error with twi rx: %x", ret_code); break;
			}
		}
		// only on busy loop around, if not break
		if (ret_code == NRFX_ERROR_BUSY) {
			sd_app_evt_wait();
		}
		else {
			break;
		}
	}

	if (ret_code == NRFX_SUCCESS) {
		while (true) {
			sd_app_evt_wait();
			if (_eventRead) break;
			if (_eventError) break;
		}
		// TODO: use time outs to break out of the following loops
		while (nrfx_twi_is_busy(&_twi)) {
			sd_app_evt_wait();
		}
		while (true) {
			sd_app_evt_wait();
			if (_eventRead) break;
			if (_eventError) break;
		}
		// if (_eventRead) {
		//	LOGd("Event read");
		//}
		if (_eventError) {
			LOGd("Event error");
		}
		length = xfer.primary_length;
		// for (int i = 0; i < (int)length; ++i) {
		//	LOGi("Read: 0x%02x", xfer.p_primary_buf[i]);
		//}
	}
}

void Twi::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_TWI_INIT: {
			TYPIFY(EVT_TWI_INIT) twi = *(TYPIFY(EVT_TWI_INIT)*)event.data;
			initBus(twi);
			break;
		}
		case CS_TYPE::EVT_TWI_WRITE: {
			TYPIFY(EVT_TWI_WRITE)* twi = (TYPIFY(EVT_TWI_WRITE)*)event.data;
			write(twi->address, twi->buf, twi->length, twi->stop);
			break;
		}
		case CS_TYPE::EVT_TWI_READ: {
			TYPIFY(EVT_TWI_READ)* twi = (TYPIFY(EVT_TWI_READ)*)event.data;
			size_t length             = twi->length;
			read(twi->address, twi->buf, length);
			twi->length = length;
			break;
		}
		default: break;
	}
}
