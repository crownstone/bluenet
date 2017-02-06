#include "cfg/cs_Boards.h"
#include "nrf_error.h"
#include "cfg/cs_DeviceTypes.h"

// overwrite the type defined by the board if the DEVICE_TYPE was defined in the config
#ifdef DEVICE_TYPE
#define ASSIGN_DEVICE_TYPE(type) DEVICE_TYPE
#else
#define ASSIGN_DEVICE_TYPE(type) type
#endif

void asCrownstone(boards_config_t* p_config) {
	p_config->pinGpioPwm           = 12;
	p_config->pinGpioRelayOn       = 10;
	p_config->pinGpioRelayOff      = 11;
	p_config->pinAinCurrent        = 7;
	p_config->pinAinVoltage        = 6;
	p_config->pinGpioRx            = 15;
	p_config->pinGpioTx            = 16;

	p_config->flags.hasRelay       = true;
	p_config->flags.pwmInverted    = true;
	p_config->flags.hasSerial      = true;
	p_config->flags.hasLed         = false;

	p_config->deviceType           = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

//	p_config->voltageMultiplier   = ; // tbd
//	p_config->currentMultiplier   = ; // tbd
//	p_config->voltageZero         = ; // tbd
//	p_config->currentZero         = ; // tbd
//	p_config->powerZero           = ; // tbd
}

void asACR01B1A(boards_config_t* p_config) {
	p_config->pinGpioPwm           = 8;
	p_config->pinGpioRelayOn       = 6;
	p_config->pinGpioRelayOff      = 7;
	p_config->pinAinCurrent        = 2;
	p_config->pinAinVoltage        = 1;
	p_config->pinGpioRx            = 20;
	p_config->pinGpioTx            = 19;
	p_config->pinLedRed            = 10;
	p_config->pinLedGreen          = 9;

	p_config->flags.hasRelay       = true;
	p_config->flags.pwmInverted    = false;
	p_config->flags.hasSerial      = false;
	p_config->flags.hasLed         = true;

	p_config->deviceType           = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_BUILTIN);

	p_config->voltageMultiplier   = 0.2f;
	p_config->currentMultiplier   = 0.0045f;
	p_config->voltageZero         = 2003;
	p_config->currentZero         = 1997;
	p_config->powerZero           = 1500;
}

void asACR01B2A(boards_config_t* p_config) {
	p_config->pinGpioPwm           = 8;
	p_config->pinGpioRelayOn       = 6;
	p_config->pinGpioRelayOff      = 7;
	p_config->pinAinCurrent        = 2;
	p_config->pinAinVoltage        = 1;
	p_config->pinGpioRx            = 20;
	p_config->pinGpioTx            = 19;
	p_config->pinLedRed            = 10;
	p_config->pinLedGreen          = 9;

	p_config->flags.hasRelay       = true;
	p_config->flags.pwmInverted    = false;
	p_config->flags.hasSerial      = false;
	p_config->flags.hasLed         = true;

	p_config->deviceType           = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

	p_config->voltageMultiplier   = 0.2f;
	p_config->currentMultiplier   = 0.0045f;
	p_config->voltageZero         = 2003;
	p_config->currentZero         = 1997;
	p_config->powerZero           = 1500;
}

void asPluginFlexprint(boards_config_t* p_config) {
	p_config->pinGpioPwm           = 15;
	p_config->pinGpioRelayOn       = 16;
	p_config->pinGpioRelayOff      = 17;
	p_config->pinAinCurrent        = 3;
	p_config->pinAinVoltage        = 2;
	p_config->pinGpioRx            = 6;
	p_config->pinGpioTx            = 7;

	p_config->flags.hasRelay       = true;
	p_config->flags.pwmInverted    = true;
	p_config->flags.hasSerial      = false;
	p_config->flags.hasLed         = false;

	p_config->deviceType           = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

//	p_config->voltageMultiplier   = ; // tbd
//	p_config->currentMultiplier   = ; // tbd
//	p_config->voltageZero         = ; // tbd
//	p_config->currentZero         = ; // tbd
//	p_config->powerZero           = ; // tbd
}

void asPca10036(boards_config_t* p_config) {
	p_config->pinGpioPwm           = 17;
	p_config->pinGpioRelayOn       = 11; // something unused
	p_config->pinGpioRelayOff      = 12; // something unused
	p_config->pinAinCurrent        = 0; // something unused
	p_config->pinAinVoltage        = 1; // something unused
	p_config->pinGpioRx            = 8;
	p_config->pinGpioTx            = 6;
	p_config->pinLedRed            = 19;
	p_config->pinLedGreen          = 20;

	p_config->deviceType           = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

	p_config->flags.hasRelay       = false;
	p_config->flags.pwmInverted    = true;
	p_config->flags.hasSerial      = true;
	p_config->flags.hasLed         = true;

	p_config->voltageMultiplier   = 0; // set to 0 to disable sampling checks
	p_config->currentMultiplier   = 0; // set to 0 to disable sampling checks
	p_config->voltageZero         = 0; // something
	p_config->currentZero         = 0; // something
	p_config->powerZero           = 0; // something
}

void asGuidestone(boards_config_t* p_config) {
//	p_config->pinGpioPwm           = ; // unused
//	p_config->pinGpioRelayOn       = ; // unused
//	p_config->pinGpioRelayOff      = ; // unused
//	p_config->pinAinCurrent        = ; // unused
//	p_config->pinAinVoltage        = ; // unused
//	p_config->pinGpioRx            = ; // unused
//	p_config->pinGpioTx            = ; // unused
//	p_config->pinLedRed            = ; // unused
//	p_config->pinLedGreen          = ; // unused

	p_config->flags.hasRelay       = false;
	p_config->flags.pwmInverted    = false;
	p_config->flags.hasSerial      = false;
	p_config->flags.hasLed         = false;

	p_config->deviceType           = ASSIGN_DEVICE_TYPE(DEVICE_GUIDESTONE);

//	p_config->voltageMultiplier   = ; // unused
//	p_config->currentMultiplier   = ; // unused
//	p_config->voltageZero         = ; // unused
//	p_config->currentZero         = ; // unused
//	p_config->powerZero           = ; // unused
}

//void as(boards_config_t* p_config) {
//	p_config->pinGpioPwm           = ;
//	p_config->pinGpioRelayOn       = ;
//	p_config->pinGpioRelayOff      = ;
//	p_config->pinAinCurrent        = ;
//	p_config->pinAinVoltage        = ;
//	p_config->pinGpioRx            = ;
//	p_config->pinGpioTx            = ;
//	p_config->pinLedRed            = ;
//	p_config->pinLedGreen          = ;
//
//	p_config->flags.hasRelay       = ;
//	p_config->flags.switchInversed = ;
//	p_config->flags.hasSerial      = ;
//	p_config->flags.hasLed         = ;
//}

uint32_t configure_board(boards_config_t* p_config) {

	uint32_t _hardwareBoard = NRF_UICR->CUSTOMER[0];
	if (_hardwareBoard == 0xFFFFFFFF) {
		_hardwareBoard = ACR01B2C;
	}

	switch(_hardwareBoard) {
	case ACR01B1A:
	case ACR01B1B:
	case ACR01B1C:
	case ACR01B1D:
		asACR01B1A(p_config);
		break;

	case ACR01B2A:
	case ACR01B2B:
	case ACR01B2C:
		asACR01B2A(p_config);
		break;

	case CROWNSTONE5:
		asCrownstone(p_config);
		break;

	case PLUGIN_FLEXPRINT_01:
		asPluginFlexprint(p_config);
		break;

	case GUIDESTONE:
		asGuidestone(p_config);
		break;

	case PCA10036:
	case PCA10040:
		asPca10036(p_config);
		break;

	default:
		// undefined board layout !!!
		asACR01B2A(p_config);
		return NRF_ERROR_INVALID_PARAM;
	}

	return NRF_SUCCESS;

}
