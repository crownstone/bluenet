/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Twi.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>

#include <nrfx_twi.h>

#define TWI_INSTANCE_ID 0

const nrfx_twi_t Twi::_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

void twi_event_handler(nrfx_twi_evt_t const *p_event, void *p_context) {
}

Twi::Twi(): EventListener() {
	EventDispatcher::getInstance().addListener(this);

	_buf = NULL;
	_bufSize = 0;
}

void Twi::init(uint8_t pin_scl, uint8_t pin_sda, uint8_t address) {
	LOGi("Init TWI");
	_config.scl = pin_scl;
	_config.sda = pin_sda;
	_config.frequency = NRF_TWI_FREQ_100K;
	_config.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;
	_config.hold_bus_uninit = false;
	_address = address;
	nrfx_twi_init(&_twi, &_config, twi_event_handler, NULL);

	// minimal buffer for reading just a single value
	// TODO: make this also into a circular buffer
	if (_buf == NULL) {
		_bufSize = 1;
		_buf = new uint8_t[_bufSize];
	}
}

void Twi::write(uint8_t *data, size_t length) {
	LOGi("Write");
	nrfx_twi_enable(&_twi);

	bool no_stop = true;
	nrfx_twi_tx(&_twi, _address, data, length, no_stop);

	nrfx_twi_disable(&_twi);
}

void Twi::read(uint8_t *data, size_t length) {
	LOGi("Read");
	nrfx_twi_enable(&_twi);

	nrfx_twi_rx(&_twi, _address, data, length);

	nrfx_twi_disable(&_twi);
}

void Twi::tick() {
	read(_buf, _bufSize);
	TYPIFY(EVT_TWI_UPDATE) twiData;
	twiData = _buf[0];
	event_t(CS_TYPE::EVT_TWI_UPDATE, &twiData, sizeof(twiData));
}

void Twi::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_TWI_INIT: {
		// TODO: Obtain from message
		uint8_t pin_scl = 31;
		uint8_t pin_sda = 32;
		uint8_t address = 10;
		init(pin_scl, pin_sda, address);
		break;
	}
	default:
		break;
	}
}
