#include <drivers/cs_Gpio.h>

static void gpioEventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t polarity) {
	LOGi("GPIO event");
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
		_pins.push_back(board.pinGpio[g_GPIO_PIN1_INDEX]);
	}
	if (g_GPIO_PIN2_INDEX != -1) {
		_pins.push_back(board.pinGpio[g_GPIO_PIN2_INDEX]);
	}
	if (g_GPIO_PIN3_INDEX != -1) {
		_pins.push_back(board.pinGpio[g_GPIO_PIN3_INDEX]);
	}
	if (g_GPIO_PIN4_INDEX != -1) {
		_pins.push_back(board.pinGpio[g_GPIO_PIN4_INDEX]);
	}

	// Buttons can be reached as GPIO 5-8 as well
	if (g_BUTTON1_INDEX != -1) {
		_pins.push_back(board.pinButton[g_BUTTON1_INDEX]);
	}
	if (g_BUTTON2_INDEX != -1) {
		_pins.push_back(board.pinButton[g_BUTTON2_INDEX]);
	}
	if (g_BUTTON3_INDEX != -1) {
		_pins.push_back(board.pinButton[g_BUTTON3_INDEX]);
	}
	if (g_BUTTON4_INDEX != -1) {
		_pins.push_back(board.pinButton[g_BUTTON4_INDEX]);
	}

	_initialized = true;
	LOGi("Configured %i GPIO pins", _pins.size());
}

void Gpio::configure(uint8_t pin_index, GpioDirection direction, GpioPullResistor pull, GpioPolarity polarity) {
	if (pin_index >= _pins.size()) {
		LOGi("Pin index does not exist");
		return;
	}

	uint32_t pin = _pins[pin_index];

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
			if (!nrf_drv_gpiote_is_init()) {
				err_code = nrf_drv_gpiote_init();
				if (err_code != NRF_SUCCESS) {
					LOGw("Error initializing GPIOTE");
					return;
				}
			}

			nrf_drv_gpiote_in_config_t config;
			switch(pull) {
				case GpioPolarity::HITOLO:
					config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
					break;
				case GpioPolarity::LOTOHI:
					config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
					break;
				case GpioPolarity::TOGGLE:
					config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
					break;
				default:
					LOGw("Huh? No such pull registor construction exists");
					return;
			}
			config.pull = nrf_pull;

			LOGi("Register pin %i with event handler", pin);

			err_code = nrf_drv_gpiote_in_init(pin, &config, gpio_event_handler);
			if (err_code != NRF_SUCCESS) {
				LOGw("Cannot initialize GPIOTE on this pin");
				return;
			}
			nrf_drv_gpiote_in_event_enable(pin, true);

			break;
		default:
			LOGw("Unknown pin action");
			break;
	}
}

/*
 * We just write, we assume the user has already configured the pin as output and with desired pull-up, etc.
 */
void Gpio::write(uint8_t pin_index, uint8_t value) {
	if (pin_index >= _pins.size()) {
		LOGi("Pin index does not exist");
		return;
	}

	// to lazy to store writeable flag with each pin, can be done for buttons e.g.
	uint32_t pin = _pins[pin_index];
	LOGi("Write value %i to pin %i", value, pin);
	nrf_gpio_pin_write(pin, value);
}

void Gpio::handleEvent(event_t& event) {
	switch(event.type) {
		case CS_TYPE::EVT_GPIO_REGISTER: {
			TYPIFY(EVT_GPIO_REGISTER) gpio = *(TYPIFY(EVT_GPIO_REGISTER)*)event.data;
			initBus(gpio);
			break;
		}
		case CS_TYPE::EVT_GPIO_WRITE: {
			TYPIFY(EVT_GPIO_WRITE)* gpio = (TYPIFY(EVT_GPIO_WRITE)*)event.data;
			write(gpio->pin);
			break;
		}
		case CS_TYPE::EVT_GPIO_READ: {
			TYPIFY(EVT_GPIO_READ)* gpio = (TYPIFY(EVT_GPIO_READ)*)event.data;
			size_t length = gpio->length;
			read(gpio->address, gpio->buf, length);
			gpio->length = length;
			break;
		}
		default:
			break;
	}
}
