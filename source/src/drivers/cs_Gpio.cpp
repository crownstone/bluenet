#include <cfg/cs_AutoConfig.h>
#include <drivers/cs_Gpio.h>
#include <events/cs_EventDispatcher.h>

static void gpioEventHandler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t polarity) {
	Gpio::getInstance().registerEvent(pin);
}

Gpio& Gpio::getInstance() {
	static Gpio instance;
	return instance;
}

Gpio::Gpio(): EventListener() {
	EventDispatcher::getInstance().addListener(this);
}

/*
 * Initialize GPIO with the board configuration. If there are button pins defined (as is the case on dev. boards) we
 * can actually control these from other modules. We limit here the number of pins that can be controlled to not be
 * accidentally driving the relay or the IGBTs.
 */
void Gpio::init(const boards_config_t & board) {
	if (g_GPIO_PIN1_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinGpio[g_GPIO_PIN1_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_GPIO_PIN2_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinGpio[g_GPIO_PIN2_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_GPIO_PIN3_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinGpio[g_GPIO_PIN3_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_GPIO_PIN4_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinGpio[g_GPIO_PIN4_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}

	// Buttons can be reached as GPIO 5-8 as well
	if (g_BUTTON1_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinButton[g_BUTTON1_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_BUTTON2_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinButton[g_BUTTON2_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_BUTTON3_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinButton[g_BUTTON3_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_BUTTON4_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinButton[g_BUTTON4_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	
	// Leds can be reached as GPIO as well
	if (g_LED1_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinLed[g_LED1_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_LED2_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinLed[g_LED2_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_LED3_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinLed[g_LED3_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}
	if (g_LED4_INDEX != -1) {
		pin_info_t pin_info;
		pin_info.pin = board.pinLed[g_LED4_INDEX];
		pin_info.event = false;
		_pins.push_back(pin_info);
	}

	_initialized = true;
	LOGi("Configured %i GPIO pins", _pins.size());
}

void Gpio::configure(uint8_t pin_index, GpioDirection direction, GpioPullResistor pull, GpioPolarity polarity) {
	if (pin_index >= _pins.size()) {
		LOGi("Pin index does not exist");
		return;
	}

	uint32_t pin = _pins[pin_index].pin;

	nrf_gpio_pin_pull_t nrf_pull;
	switch(pull) {
		case GpioPullResistor::NONE:
			nrf_pull = NRF_GPIO_PIN_NOPULL;
			break;
		case GpioPullResistor::UP:
			nrf_pull = NRF_GPIO_PIN_PULLUP;
			break;
		case GpioPullResistor::DOWN:
			nrf_pull = NRF_GPIO_PIN_PULLDOWN;
			break;
		default:
			LOGw("Huh? No such pull registor construction exists");
			return;
	}
	LOGi("Set pin with pull none / up / down setting: %i", nrf_pull);

	switch(direction) {
		case GpioDirection::INPUT:
			LOGi("Configure pin %i as input", pin);
			nrf_gpio_cfg_input(pin, nrf_pull);
			break;
		case GpioDirection::OUTPUT:
			LOGi("Configure pin %i as output", pin);
			nrf_gpio_cfg_output(pin);
			break;
		case GpioDirection::SENSE:
			// enable GPIOTE in general
			uint32_t err_code;
			if (!nrfx_gpiote_is_init()) {
				err_code = nrfx_gpiote_init();
				if (err_code != NRF_SUCCESS) {
					LOGw("Error initializing GPIOTE");
					return;
				}
			}

			nrfx_gpiote_in_config_t config;
			switch(polarity) {
				case GpioPolarity::HITOLO:
					config.sense = NRF_GPIOTE_POLARITY_HITOLO;
					break;
				case GpioPolarity::LOTOHI:
					config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
					break;
				case GpioPolarity::TOGGLE:
					config.sense = NRF_GPIOTE_POLARITY_TOGGLE;
					break;
				default:
					LOGw("Huh? No such polarity exists");
					return;
			}
			config.pull = nrf_pull;
			config.hi_accuracy = true;
			config.is_watcher = false;
			config.skip_gpio_setup = false;

			LOGi("Register pin %i using polarity %i with event handler", pin, polarity);

			err_code = nrfx_gpiote_in_init(pin, &config, gpioEventHandler);
			if (err_code != NRF_SUCCESS) {
				LOGw("Cannot initialize GPIOTE on this pin");
				return;
			}
			nrfx_gpiote_in_event_enable(pin, true);

			break;
		default:
			LOGw("Unknown pin action");
			break;
	}
}

/*
 * We just write, we assume the user has already configured the pin as output and with desired pull-up, etc.
 */
void Gpio::write(uint8_t pin_index, uint8_t *buf, uint8_t & length) {
	if (pin_index >= _pins.size()) {
		LOGi("Pin index does not exist");
		return;
	}

	// to lazy to store writeable flag with each pin, can be done for buttons e.g.
	uint32_t pin = _pins[pin_index].pin;

	// TODO: limit length
	for (int i = 0; i < length; ++i) {
		LOGi("Write value %i to pin %i", buf[i], pin);
		nrf_gpio_pin_write(pin, buf[i]);
	}
}

void Gpio::read(uint8_t pin_index, uint8_t *buf, uint8_t & length) {
	if (pin_index >= _pins.size()) {
		LOGi("Pin index does not exist");
		return;
	}

	// to lazy to store readeable flag with each pin, can be done for buttons e.g.
	uint32_t pin = _pins[pin_index].pin;

	// TODO: limit length
	for (int i = 0; i < length; ++i) {
		buf[i] = nrf_gpio_pin_read(pin);
		LOGi("Read value %i from pin %i", buf[i], pin);
	}
}

/*
 * Called from interrupt service routine, only write which pin is fired and return immediately.
 */
void Gpio::registerEvent(uint8_t pin) {
	//LOGi("GPIO event on pin %i", pin);
	for (uint8_t i = 0; i < _pins.size(); ++i) {
		if (_pins[i].pin == pin) {
			_pins[i].event = true;
			break;
		}
	}
}

void Gpio::tick() {
	for (uint8_t i = 0; i < _pins.size(); ++i) {
		if (_pins[i].event) {
			_pins[i].event = false;
			TYPIFY(EVT_GPIO_UPDATE) gpio;
			// we send back the pin index, not the pin number
			gpio.pin_index = i;
			gpio.length = 0;
			LOGi("Send GPIO event on pin %i at index %i", _pins[i], gpio.pin_index);
			event_t event(CS_TYPE::EVT_GPIO_UPDATE, &gpio, sizeof(gpio));
			EventDispatcher::getInstance().dispatch(event);
		}
	}
}

void Gpio::handleEvent(event_t& event) {
	switch(event.type) {
		case CS_TYPE::EVT_GPIO_INIT: {
			LOGi("Configure GPIO pin");
			TYPIFY(EVT_GPIO_INIT) gpio = *(TYPIFY(EVT_GPIO_INIT)*)event.data;
			GpioPolarity polarity = (GpioPolarity)gpio.polarity;
			GpioDirection direction = (GpioDirection)gpio.direction;
			GpioPullResistor pull = (GpioPullResistor)gpio.pull;
			configure(gpio.pin_index, direction, pull, polarity);
			break;
		}
		case CS_TYPE::EVT_GPIO_WRITE: {
			TYPIFY(EVT_GPIO_WRITE) gpio = *(TYPIFY(EVT_GPIO_WRITE)*)event.data;
			if (gpio.buf == NULL) {
				LOGw("Buffer is null");
				return;
			}
			write(gpio.pin_index, gpio.buf, gpio.length);
			break;
		}
		case CS_TYPE::EVT_GPIO_READ: {
			TYPIFY(EVT_GPIO_READ) gpio = *(TYPIFY(EVT_GPIO_READ)*)event.data;
			read(gpio.pin_index, gpio.buf, gpio.length);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			tick();
			break;
		}
		default:
			break;
	}
}
