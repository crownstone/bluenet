#include "cfg/cs_Boards.h"
#include "nrf_error.h"
#include "cfg/cs_DeviceTypes.h"

// overwrite the type defined by the board if the DEVICE_TYPE was defined in the config
#ifdef DEVICE_TYPE
#define ASSIGN_DEVICE_TYPE(type) DEVICE_TYPE
#else
#define ASSIGN_DEVICE_TYPE(type) type
#endif

void asACR01B1A(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;
	p_config->pinAinCurrent                      = 2;
	p_config->pinAinVoltage                      = 1;
//	p_config->pinAinZeroRef                      = -1; // Non existing
	p_config->pinAinPwmTemp                      = 3;
	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;
	p_config->pinLedRed                          = 10;
	p_config->pinLedGreen                        = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = false;

	p_config->deviceType                         = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_BUILTIN);

	p_config->voltageMultiplier                  = 0.2f;
	p_config->currentMultiplier                  = 0.0044f;
	p_config->voltageZero                        = 1993;
	p_config->currentZero                        = 1980;
	p_config->powerZero                          = 3504;
	p_config->voltageRange                       = 1200; // 0V - 1.2V
	p_config->currentRange                       = 1200; // 0V - 1.2V

	p_config->pwmTempVoltageThreshold            = 0.76; // About 1.5kOhm --> 90-100C
	p_config->pwmTempVoltageThresholdDown        = 0.41; // About 0.7kOhm --> 70-95C

	p_config->minTxPower                         = -20; // higher tx power for builtins
}


void asACR01B6C(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;
	p_config->pinAinCurrent                      = 2;
	p_config->pinAinVoltage                      = 1;
	p_config->pinAinZeroRef                      = 0;
	p_config->pinAinPwmTemp                      = 3;
	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;
	p_config->pinLedRed                          = 10;
	p_config->pinLedGreen                        = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_BUILTIN);

	p_config->voltageMultiplier                  = 0.171f; // TODO: calibrate
	p_config->currentMultiplier                  = 0.0042f; // TODO: calibrate
	p_config->voltageZero                        = -99; // TODO: calibrate
	p_config->currentZero                        = -270; // TODO: calibrate
	p_config->powerZero                          = 8000; // TODO: calibrate
	p_config->voltageRange                       = 1200; // TODO: calibrate
	p_config->currentRange                       = 600; // TODO: calibrate

	p_config->pwmTempVoltageThreshold            = 0.7;  // About 50 degrees C
	p_config->pwmTempVoltageThresholdDown        = 0.25; // About 90 degrees C

	p_config->minTxPower                         = -20; // higher tx power for builtins
}


void asACR01B2A(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;
	p_config->pinAinCurrent                      = 2;
	p_config->pinAinVoltage                      = 1;
//	p_config->pinAinZeroRef                      = -1; // Non existing
	p_config->pinAinPwmTemp                      = 3;
	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;
	p_config->pinLedRed                          = 10;
	p_config->pinLedGreen                        = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = false;

	p_config->deviceType                         = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

	p_config->voltageMultiplier                  = 0.2f;
	p_config->currentMultiplier                  = 0.0045f;
	p_config->voltageZero                        = 2003; // 2010 seems better?
	p_config->currentZero                        = 1997; // 1991 seems better?
	p_config->powerZero                          = 1500;
	p_config->voltageRange                       = 1200; // 0V - 1.2V
	p_config->currentRange                       = 1200; // 0V - 1.2V

	p_config->pwmTempVoltageThreshold            = 0.76; // About 1.5kOhm --> 90-100C
	p_config->pwmTempVoltageThresholdDown        = 0.41; // About 0.7kOhm --> 70-95C

	p_config->minTxPower                         = -20;
}

void asACR01B2G(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 8;
	p_config->pinGpioRelayOn                     = 6;
	p_config->pinGpioRelayOff                    = 7;
	p_config->pinAinCurrent                      = 2;
	p_config->pinAinVoltage                      = 1;
	p_config->pinAinZeroRef	                     = 0;
	p_config->pinAinPwmTemp                      = 3;
	p_config->pinGpioRx                          = 20;
	p_config->pinGpioTx                          = 19;
	p_config->pinLedRed                          = 10;
	p_config->pinLedGreen                        = 9;

	p_config->flags.hasRelay                     = true;
	p_config->flags.pwmInverted                  = false;
	p_config->flags.hasSerial                    = false;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = false;
	p_config->flags.hasAdcZeroRef                = true;
//	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = true;

	p_config->deviceType                         = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

	p_config->voltageMultiplier                  = 0.171f;  // Calibrated by noisy data from 1 crownstone

//	p_config->currentMultiplier                  = 0.0037f; // Calibrated by noisy data from 1 crownstone
	p_config->currentMultiplier                  = 0.0042f; // Calibrated by noisy data from 2 crownstones

	p_config->voltageZero                        = -99;     // Calibrated by noisy data from 1 crownstone
	p_config->currentZero                        = -270;    // Calibrated by noisy data from 1 crownstone

	p_config->powerZero                          = 8000;   // Calibrated by noisy data from 2 crownstones

	p_config->voltageRange                       = 1200; // 0V - 1.2V, or -1.2V - 1.2V around zeroRef pin // voltage ranges between 0.54 and 2.75, ref = 1.65
//	p_config->currentRange                       = 1800; // 0V - 1.8V, or -1.8V - 1.8V around zeroRef pin // Able to measure up to about 20A.
//	p_config->currentRange                       = 1200; // 0V - 1.2V, or -1.2V - 1.2V around zeroRef pin // Able to measure up to about 13A.
	p_config->currentRange                       = 600;  // 0V - 0.6V, or -0.6V - 0.6V around zeroRef pin // Able to measure up to about 6A.
//	p_config->currentRange                       = 1800;

	p_config->pwmTempVoltageThreshold            = 0.7;  // About 50 degrees C
	p_config->pwmTempVoltageThresholdDown        = 0.25; // About 90 degrees C

	p_config->minTxPower                         = -20;
}

void asPca10036(boards_config_t* p_config) {
	p_config->pinGpioPwm                         = 17;
	p_config->pinGpioRelayOn                     = 11; // something unused
	p_config->pinGpioRelayOff                    = 12; // something unused
	p_config->pinAinCurrent                      = 1; // gpio3
	p_config->pinAinVoltage                      = 2; // gpio4
	p_config->pinAinZeroRef	                     = 4; // gpio28
	p_config->pinAinPwmTemp                      = 0; // gpio2
	p_config->pinGpioRx                          = 8;
	p_config->pinGpioTx                          = 6;
	p_config->pinLedRed                          = 19;
	p_config->pinLedGreen                        = 20;

	p_config->flags.hasRelay                     = false;
	p_config->flags.pwmInverted                  = true;
	p_config->flags.hasSerial                    = true;
	p_config->flags.hasLed                       = true;
	p_config->flags.ledInverted                  = true;
	p_config->flags.hasAdcZeroRef                = false;
	p_config->flags.pwmTempInverted              = false;

	p_config->deviceType                         = ASSIGN_DEVICE_TYPE(DEVICE_CROWNSTONE_PLUG);

	p_config->voltageMultiplier                  = 0.0; // set to 0 to disable sampling checks
	p_config->currentMultiplier                  = 0.0; // set to 0 to disable sampling checks
	p_config->voltageZero                        = 1000; // something
	p_config->currentZero                        = 1000; // something
	p_config->powerZero                          = 0; // something
	p_config->voltageRange                       = 3600; // 0V - 3.6V
	p_config->currentRange                       = 3600; // 0V - 3.6V

	p_config->pwmTempVoltageThreshold            = 2.0; // something
	p_config->pwmTempVoltageThresholdDown        = 1.0; // something

	p_config->minTxPower                         = -40;
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
	
	p_config->minTxPower          = -20;
}

uint32_t configure_board(boards_config_t* p_config) {

	uint32_t _hardwareBoard = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	if (_hardwareBoard == 0xFFFFFFFF) {
		_hardwareBoard = ACR01B2C;
	}

	switch(_hardwareBoard) {
	case ACR01B1A:
	case ACR01B1B:
	case ACR01B1C:
	case ACR01B1D:
	case ACR01B1E:
		asACR01B1A(p_config);
		break;

	case ACR01B6C:
		asACR01B6C(p_config);
		break;

	case ACR01B2A:
	case ACR01B2B:
	case ACR01B2C:
		asACR01B2A(p_config);
		break;

	case ACR01B2E:
	case ACR01B2G:
		asACR01B2G(p_config);
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
