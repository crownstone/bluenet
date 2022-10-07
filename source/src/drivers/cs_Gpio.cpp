#include <ble/cs_Nordic.h>
#include <cfg/cs_AutoConfig.h>
#include <drivers/cs_Gpio.h>
#include <events/cs_EventDispatcher.h>
#include <logging/cs_Logger.h>

#define LOGGpioInterrupt LOGvv

static void gpioEventHandler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t polarity) {
	Gpio::getInstance().registerEvent(pin);
}

Gpio& Gpio::getInstance() {
	static Gpio instance;
	return instance;
}

Gpio::Gpio() : EventListener() {
	EventDispatcher::getInstance().addListener(this);
}

/*
 * Initialize GPIO with the board configuration. If there are button pins defined (as is the case on dev. boards) we
 * can actually control these from other modules. We limit here the number of pins that can be controlled to not be
 * accidentally driving the relay or the IGBTs.
 *
 * Interrupts are not put on the app scheduler, but instead pins are marked to have an interrupt.
 * Each tick, all pins are checked whether they are marked to have an interrupt. If so, EVT_GPIO_UPDATE is dispatched.
 */
void Gpio::init(const boards_config_t& board) {

	int activePins = 0;
	for (int i = 0; i < GPIO_INDEX_COUNT; i++) {
		_pins[i].event = false;
		if (board.pinGpio[i] != PIN_NONE) {
			activePins++;
		}
	}
	for (int i = 0; i < BUTTON_COUNT; i++) {
		_pins[GPIO_INDEX_COUNT + i].event = false;
		if (board.pinButton[i] != PIN_NONE) {
			activePins++;
		}
	}
	for (int i = 0; i < LED_COUNT; i++) {
		_pins[GPIO_INDEX_COUNT + BUTTON_COUNT + i].event = false;
		if (board.pinLed[i] != PIN_NONE) {
			activePins++;
		}
	}

	_initialized = true;
	_boardConfig = &board;
	LOGi("Configured %i GPIO pins", activePins);
}

bool Gpio::pinExists(uint8_t pinIndex) {
	if (!_initialized) {
		LOGi("GPIO driver not initialized");
		return false;
	}
	if (pinIndex >= TOTAL_PIN_COUNT) {
		LOGw("Pin index %i out of max pin range (max %i)", pinIndex, TOTAL_PIN_COUNT);
		return false;
	}
	if (pinIndex < GPIO_INDEX_COUNT) {
		if (_boardConfig->pinGpio[pinIndex] != PIN_NONE) {
			return true;
		}
	}
	else if (pinIndex < GPIO_INDEX_COUNT + BUTTON_COUNT) {
		if (_boardConfig->pinButton[pinIndex - GPIO_INDEX_COUNT] != PIN_NONE) {
			return true;
		}
	}
	else {
		if (_boardConfig->pinLed[pinIndex - GPIO_INDEX_COUNT - BUTTON_COUNT] != PIN_NONE) {
			return true;
		}
	}
	LOGi("Pin index %i not defined for this board", pinIndex);
	return false;
}

pin_t Gpio::getPin(uint8_t pinIndex) {

	if (!pinExists(pinIndex)) {
		return PIN_NONE;
	}
	if (pinIndex < GPIO_INDEX_COUNT) {
		return _boardConfig->pinGpio[pinIndex];
	}
	else if (pinIndex < GPIO_INDEX_COUNT + BUTTON_COUNT) {
		return _boardConfig->pinButton[pinIndex - GPIO_INDEX_COUNT];
	}
	else {
		return _boardConfig->pinLed[pinIndex - GPIO_INDEX_COUNT - BUTTON_COUNT];
	}
}

bool Gpio::isLedPin(uint8_t pinIndex) {
	if (!pinExists(pinIndex)) {
		return false;
	}
	else {
		return (pinIndex >= GPIO_INDEX_COUNT + BUTTON_COUNT);
	}
}

cs_ret_code_t Gpio::configure(uint8_t pinIndex, GpioDirection direction, GpioPullResistor pull, GpioPolarity polarity) {
	pin_t pin = getPin(pinIndex);
	if (pin == PIN_NONE) {
		return ERR_NOT_FOUND;
	}
	LOGi("Configure pinIndex=%u pin=%u direction=%i pullResistor=%i polarity=%i",
		 pinIndex,
		 pin,
		 direction,
		 pull,
		 polarity);

	nrf_gpio_pin_pull_t nrfPull;
	switch (pull) {
		case GpioPullResistor::NONE: {
			nrfPull = NRF_GPIO_PIN_NOPULL;
			LOGd("  resistor: none");
			break;
		}
		case GpioPullResistor::UP: {
			nrfPull = NRF_GPIO_PIN_PULLUP;
			LOGd("  resistor: pull-up");
			break;
		}
		case GpioPullResistor::DOWN: {
			nrfPull = NRF_GPIO_PIN_PULLDOWN;
			LOGd("  resistor: pull-down");
			break;
		}
		default: {
			LOGw("Invalid pull resistor config: %i", pull);
			return ERR_WRONG_PARAMETER;
		}
	}

	// Always deinit the gpiote for this pin.
	if (nrfx_gpiote_is_init()) {
		if (_pins[pinIndex].direction == GpioDirection::SENSE) {
			LOGd("  unregister event handler for pin=%u", pin);
			nrfx_gpiote_in_uninit(pin);
		}
	}

	_pins[pinIndex].direction = 0;
	_pins[pinIndex].event = false;

	switch (direction) {
		case GpioDirection::INPUT: {
			LOGd("  direction: input");
			nrf_gpio_cfg_input(pin, nrfPull);
			_pins[pinIndex].direction = direction;
			break;
		}
		case GpioDirection::OUTPUT: {
			LOGd("  direction: output");
			nrf_gpio_cfg_output(pin);
			_pins[pinIndex].direction = direction;
			break;
		}
		case GpioDirection::SENSE: {
			LOGd("  direction: sense");
			// enable GPIOTE in general
			uint32_t nrfCode;
			if (!nrfx_gpiote_is_init()) {
				nrfCode = nrfx_gpiote_init();
				if (nrfCode != NRF_SUCCESS) {
					LOGw("Error initializing GPIOTE: nrfCode=%u", nrfCode);
					return ERR_UNSPECIFIED;
				}
			}

			nrfx_gpiote_in_config_t gpioteConfig;
			switch (polarity) {
				case GpioPolarity::HITOLO: {
					LOGd("  polarity: high to low");
					gpioteConfig.sense = NRF_GPIOTE_POLARITY_HITOLO;
					break;
				}
				case GpioPolarity::LOTOHI: {
					LOGd("  polarity: low to high");
					gpioteConfig.sense = NRF_GPIOTE_POLARITY_LOTOHI;
					break;
				}
				case GpioPolarity::TOGGLE: {
					LOGd("  polarity: toggle");
					gpioteConfig.sense = NRF_GPIOTE_POLARITY_TOGGLE;
					break;
				}
				default: {
					LOGw("Invalid polarity: %i", polarity);
					return ERR_WRONG_PARAMETER;
				}
			}
			gpioteConfig.pull            = nrfPull;

			// With high accuracy, nrfx_gpiote will make use of GPIOTE channels.
			// These channels might already be used by our own PWM class.
			gpioteConfig.hi_accuracy     = false;
			gpioteConfig.is_watcher      = false;
			gpioteConfig.skip_gpio_setup = false;

			LOGd("  register event handler");

			nrfCode = nrfx_gpiote_in_init(pin, &gpioteConfig, gpioEventHandler);
			if (nrfCode != NRF_SUCCESS) {
				LOGw("Cannot initialize GPIOTE for pin=%u nrfCode=%u", pin, nrfCode);
				return ERR_UNSPECIFIED;
			}
			nrfx_gpiote_in_event_enable(pin, true);
			_pins[pinIndex].direction = direction;
			break;
		}
		default: {
			LOGw("Invalid direction: %i", direction);
			return ERR_WRONG_PARAMETER;
		}
	}
	return ERR_SUCCESS;
}

/*
 * We just write, we assume the user has already configured the pin as output and with desired pull-up, etc.
 */
cs_ret_code_t Gpio::write(uint8_t pinIndex, uint8_t* buf, uint8_t& length) {

	pin_t pin = getPin(pinIndex);
	if (pin == PIN_NONE) {
		LOGi("Can't write pin with pin index %i", pinIndex);
		return ERR_NOT_FOUND;
	}

	// TODO: limit length
	for (int i = 0; i < length; ++i) {
		// Invert value to write if leds are inverted
		if (isLedPin(pinIndex) && _boardConfig->flags.ledInverted) {
			buf[i] = !buf[i];
		}
		LOGi("Write value %i to pin %i (pin index %i)", buf[i], pin, pinIndex);
		nrf_gpio_pin_write(pin, buf[i]);
	}
	return ERR_SUCCESS;
}

cs_ret_code_t Gpio::read(uint8_t pinIndex, uint8_t* buf, uint8_t& length) {

	pin_t pin = getPin(pinIndex);
	if (pin == PIN_NONE) {
		LOGi("Can't read pin with pin index %i", pinIndex);
		return ERR_NOT_FOUND;
	}

	// TODO: limit length
	for (int i = 0; i < length; ++i) {
		buf[i] = nrf_gpio_pin_read(pin);
		LOGi("Read value %i from pin %i (pin index %i)", buf[i], pin, pinIndex);
	}
	return ERR_SUCCESS;
}

/*
 * Called from interrupt service routine, only write which pin is fired and return immediately.
 */
void Gpio::registerEvent(pin_t pin) {
	LOGGpioInterrupt("GPIO event on pin %i", pin);
	for (uint8_t i = 0; i < GPIO_INDEX_COUNT; ++i) {
		if (_boardConfig->pinGpio[i] == pin) {
			_pins[i].event = true;
			return;
		}
	}
	for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
		if (_boardConfig->pinButton[i] == pin) {
			_pins[GPIO_INDEX_COUNT + i].event = true;
			return;
		}
	}
	for (uint8_t i = 0; i < LED_COUNT; ++i) {
		if (_boardConfig->pinLed[i] == pin) {
			_pins[GPIO_INDEX_COUNT + BUTTON_COUNT + i].event = true;
			return;
		}
	}
}

void Gpio::tick() {
	for (uint8_t i = 0; i < TOTAL_PIN_COUNT; ++i) {
		if (_pins[i].event) {
			_pins[i].event = false;
			TYPIFY(EVT_GPIO_UPDATE) gpio;
			// we send back the pin index, not the pin number
			gpio.pinIndex = i;
			gpio.length   = 0;
			LOGd("Send GPIO event at index %i", gpio.pinIndex);
			event_t event(CS_TYPE::EVT_GPIO_UPDATE, &gpio, sizeof(gpio));
			EventDispatcher::getInstance().dispatch(event);
		}
	}
}

void Gpio::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_GPIO_INIT: {
			TYPIFY(EVT_GPIO_INIT) gpio = *(TYPIFY(EVT_GPIO_INIT)*)event.data;
			GpioPolarity polarity      = (GpioPolarity)gpio.polarity;
			GpioDirection direction    = (GpioDirection)gpio.direction;
			GpioPullResistor pull      = (GpioPullResistor)gpio.pull;
			event.result.returnCode    = configure(gpio.pinIndex, direction, pull, polarity);
			break;
		}
		case CS_TYPE::EVT_GPIO_WRITE: {
			TYPIFY(EVT_GPIO_WRITE) gpio = *(TYPIFY(EVT_GPIO_WRITE)*)event.data;
			if (gpio.buf == NULL) {
				LOGw("Buffer is null");
				return;
			}
			event.result.returnCode = write(gpio.pinIndex, gpio.buf, gpio.length);
			break;
		}
		case CS_TYPE::EVT_GPIO_READ: {
			TYPIFY(EVT_GPIO_READ) gpio = *(TYPIFY(EVT_GPIO_READ)*)event.data;
			event.result.returnCode    = read(gpio.pinIndex, gpio.buf, gpio.length);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			tick();
			break;
		}
		default: break;
	}
}
