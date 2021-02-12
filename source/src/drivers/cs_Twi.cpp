/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Twi.h>

#include <ble/cs_Nordic.h>
#include <cfg/cs_AutoConfig.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>

#define TWI_INSTANCE_ID 0

const nrfx_twi_t Twi::_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

void twiEventHandler(nrfx_twi_evt_t const* event, void* context) {
	// the event handler is not used yet, it is only required if we cannot just use read()
}

Twi::Twi(): EventListener(), _init(false) {
	EventDispatcher::getInstance().addListener(this);
}

void Twi::init(const boards_config_t & board) {
	_config.sda = board.pinGpio[g_TWI_SDA_INDEX];
	_config.scl = board.pinGpio[g_TWI_SCL_INDEX];
	LOGi("Configured TWI at SDA %i and SCL %i", _config.sda, _config.scl);
}

/*
 * Only operate as master.
 */
void Twi::initBus(cs_twi_init_t& twi) {
	LOGi("Init TWI bus");
	if (_init) {
		LOGw("Already initialized!");
		return;
	}
	_config.frequency = NRF_TWI_FREQ_100K;
	_config.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;
	_config.hold_bus_uninit = false;
	nrfx_err_t ret = nrfx_twi_init(&_twi, &_config, twiEventHandler, NULL);
	if (ret != ERR_SUCCESS) {
		LOGw("Init error: %i", ret);
		return;
	}

	_init = true;
}

void Twi::write(uint8_t address, uint8_t* data, size_t length, bool stop) {
	if (!_init) {
		LOGw("Twi not initialized yet, cannot write");
		return;
	}
	LOGi("Write i2c value");
	nrfx_twi_enable(&_twi);

	nrfx_twi_tx(&_twi, address, data, length, !stop);

	nrfx_twi_disable(&_twi);
}

void Twi::read(uint8_t address, uint8_t* data, size_t & length) {
	uint16_t ret_code;
	LOGi("Read i2c value");
	nrfx_twi_enable(&_twi);

	ret_code = nrfx_twi_rx(&_twi, address, data, length);
	if (ret_code != ERR_SUCCESS) {
		LOGw("Error with twi rx");
		length = 0;
	}

	nrfx_twi_disable(&_twi);
}

void Twi::handleEvent(event_t& event) {
	switch(event.type) {
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
			size_t length = twi->length;
			read(twi->address, twi->buf, length);
			twi->length = length;
			break;
		}
		default:
			break;
	}
}
