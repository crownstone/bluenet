/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include "third/std/function.h"
//#include <vector>

//#include <common/cs_Types.h>

//#include "characteristics/cs_UuidConfig.h"
#include "drivers/cs_Storage.h"
//#include "ble/cs_BluetoothLE.h"
#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include "drivers/cs_LPComp.h"
#include "structs/buffer/cs_CircularBuffer.h"
#include "structs/buffer/cs_StackBuffer.h"
#include "structs/buffer/cs_DifferentialBuffer.h"
#include "structs/cs_PowerSamples.h"
#include "protocol/cs_MeshMessageTypes.h"

#define POWER_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** PowerService controls the relays on the Crownstone.
 *
 * The PowerService has the following functionalities:
 *   - toggle the relay and hereby switch on/off devices
 *   - control the IGBT and hereby dim devices through pulse width modulation (PWM)
 *   - measure current
 *   - measure power consumption
 *   - set a limit on the maximum current (the relay will be automatically switched off above this threshold)
 */
class PowerService : public BLEpp::Service {
public:
//	typedef function<int8_t()> func_t;

	/** Constructor for power service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	PowerService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things.
	 * Urgent matters have to be resolved immediately in interrupt service handlers.
	 */
	void tick();

	void scheduleNextTick();

	/** Switch off the relays.
	 */
	void turnOff();

	/** Switch on the relays.
	 */
	void turnOn();

	/** Dim the light by using PWM.
	 *
	 * You might need another way to dim the light! For example by only turning on for
	 * a specific duty-cycle after the detection of a zero crossing.
	 */
	void dim(uint8_t value);
protected:
	//! The characteristics in this service
	void addPWMCharacteristic();
	void addRelayCharacteristic();
	void addPowerSamplesCharacteristic();
	void addPowerConsumptionCharacteristic();
	void addCurrentLimitCharacteristic();

	/** Initializes the buffer.
	 */
	void powerSampleInit();

	/** Initializes and starts the ADC, also starts interval timer.
	 */
	void powerSampleFirstStart();

	/** Starts a new power sample burst.
	 *  Called at a low interval.
	 */
	void powerSampleStart();
	static void staticPowerSampleStart(PowerService *ptr) {
		ptr->powerSampleStart();
	}

	/** Called when the sample burst is finished.
	 *  Calculates the power usage, updates the state.
	 *  Sends the samples if the central is subscribed for that.
	 */
	void powerSampleFinish();

	/** Called at a short interval.
	 *  Reads out the buffer.
	 *  Sends the samples via notifications and/or mesh.
	 */
	void powerSampleReadBuffer();
	static void staticPowerSampleRead(PowerService *ptr) {
		ptr->powerSampleReadBuffer();
	}



	/** Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrentDone(uint8_t type);

	/** Get the stored current limit.
	 */
//	uint8_t getCurrentLimit();
//	void setCurrentLimit(uint8_t value);

	/** Get a handle to the persistent storage struct and load it from FLASH.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
//	void loadPersistentStorage();

	/** Save to FLASH.
	 */
//	void savePersistentStorage();
private:
	//! References to characteristics that need to be written from other functions
	BLEpp::Characteristic<uint8_t> *_pwmCharacteristic;
	BLEpp::Characteristic<uint8_t> *_relayCharacteristic;
	BLEpp::Characteristic<uint16_t> *_powerConsumptionCharacteristic;
	BLEpp::Characteristic<uint8_t*> *_powerSamplesCharacteristic;
//	BLEpp::Characteristic<uint8_t> *_currentLimitCharacteristic;

	app_timer_id_t _staticPowerSamplingStartTimer;
	app_timer_id_t _staticPowerSamplingReadTimer;

	buffer_ptr_t _powerSamplesBuffer; //! Buffer that holds the data for burst or continous sampling

//	DifferentialBuffer<uint32_t> _currentSampleTimestamps;
//	DifferentialBuffer<uint32_t> _voltageSampleTimestamps;
	CircularBuffer<uint16_t> _currentSampleCircularBuf;
	CircularBuffer<uint16_t> _voltageSampleCircularBuf;
#if CHAR_MESHING == 1
	power_samples_mesh_message_t* _powerSamplesMeshMsg;
#endif
	uint16_t _powerSamplesCount;
//	uint16_t _lastPowerSample;

	PowerSamples _powerSamples;
	bool _powerSamplesProcessed;

	uint8_t _currentLimitVal;

//	pstorage_handle_t _storageHandle;
//	ps_power_service_t _storageStruct;

//	bool _currentLimitInitialized;

	void sampleCurrent(uint8_t type);
};
