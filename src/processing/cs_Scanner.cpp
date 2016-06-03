/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 2, 2015
 * License: LGPLv3+
 */
#include "processing/cs_Scanner.h"

#include <mesh/cs_MeshControl.h>
#include <storage/cs_Settings.h>

#include <cfg/cs_DeviceTypes.h>
#include <ble/cs_DoBotsManufac.h>

#include <events/cs_EventDispatcher.h>

Scanner::Scanner() :
	_opCode(SCAN_START),
	_scanning(false),
	_running(false),
	_scanDuration(SCAN_DURATION),
	_scanSendDelay(SCAN_SEND_DELAY),
	_scanBreakDuration(SCAN_BREAK_DURATION),
	_scanFilter(SCAN_FILTER),
	_filterSendFraction(SCAN_FILTER_SEND_FRACTION),
	_scanCount(0),
	_stack(NULL)
{

	_scanResult = new ScanResult();

	//! [29.01.16] the scan result needs it's own buffer, not the master buffer,
	//! since it is now decoupled from writing to a characteristic.
	//! if we used the master buffer we would overwrite the scan results
	//! if we write / read from a characteristic that uses the master buffer
	//! during a scan!
	_scanResult->assign(_scanBuffer, sizeof(_scanBuffer));

	Settings& settings = Settings::getInstance();
	settings.get(CONFIG_SCAN_DURATION, &_scanDuration);
	settings.get(CONFIG_SCAN_SEND_DELAY, &_scanSendDelay);
	settings.get(CONFIG_SCAN_BREAK_DURATION, &_scanBreakDuration);
	settings.get(CONFIG_SCAN_FILTER, &_scanFilter);

	EventDispatcher::getInstance().addListener(this);

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Scanner::staticTick);

	LOGe("scanner timer id: %d", _appTimerId);
}

void Scanner::setStack(Nrf51822BluetoothStack* stack) {
	_stack = stack;
}

void Scanner::manualStartScan() {
	if (!_stack) {
		LOGe("forgot to assign stack!");
		return;
	}

	LOGi("Init scan result");
	_scanResult->clear();
	_scanning = true;

	if (!_stack->isScanning()) {
		LOGw("start scanning...");
		_stack->startScanning();
	}
}

void Scanner::manualStopScan() {
	if (!_stack) {
		LOGe("forgot to assign stack!");
		return;
	}

	_scanning = false;

	if (_stack->isScanning()) {
		LOGw("stop scanning...");
		_stack->stopScanning();
	}
}

bool Scanner::isScanning() {
	if (!_stack) {
		LOGe("forgot to assign stack!");
		return false;
	}

	return _scanning && _stack->isScanning();
}

ScanResult* Scanner::getResults() {
	return _scanResult;
}

void Scanner::staticTick(Scanner* ptr) {
	ptr->executeScan();
}

void Scanner::start() {
	if (!_running) {
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		executeScan();
	} else {
		LOGi("already scanning!");
	}
}

void Scanner::delayedStart(uint16_t delay) {
	if (!_running) {
		LOGw("delayed start by %d ms", delay);
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(delay), this);
	} else {
		LOGi("already scanning!");
	}
}

void Scanner::delayedStart() {
	if (!_running) {
		LOGw("delayed start by %d ms", _scanBreakDuration);
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);
	} else {
		LOGi("already scanning!");
	}
}

void Scanner::stop() {
	if (_running) {
		_running = false;
		_opCode = SCAN_STOP;
		LOGi("force STOP");
		manualStopScan();
		//! no need to execute scan on stop is there? we want to stop after all
	//	executeScan();
	//	_running = false;
	} else {
		LOGi("already stopped!");
	}
}

void Scanner::executeScan() {

	if (!_running) return;

	LOGi("executeScan");
	switch(_opCode) {
	case SCAN_START: {
		LOGd("START");

		//! start scanning
		manualStartScan();
		if (_filterSendFraction > 0) {
			_scanCount = (_scanCount+1) % _filterSendFraction;
		}

		//! set timer to trigger in SCAN_DURATION sec, then stop again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanDuration), this);

		_opCode = SCAN_STOP;
		break;
	}
	case SCAN_STOP: {
		LOGd("STOP");

		//! stop scanning
		manualStopScan();

		_scanResult->print();

		//! Wait SCAN_SEND_WAIT ms before sending the results, so that it can listen to the mesh before sending
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanSendDelay), this);

		_opCode = SCAN_SEND_RESULT;
		break;
	}
	case SCAN_SEND_RESULT: {
		LOGd("SCAN_SEND_RESULT");

		sendResults();

		//! Wait SCAN_BREAK ms, then start scanning again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);

		_opCode = SCAN_START;
		break;
	}
	}

}

void Scanner::sendResults() {
	if (Settings::getInstance().isEnabled(CONFIG_MESH_ENABLED)) {
		MeshControl::getInstance().sendScanMessage(_scanResult->getList()->list, _scanResult->getSize());
	}

	buffer_ptr_t buffer;
	uint16_t length;
	_scanResult->getBuffer(buffer, length);

	EventDispatcher::getInstance().dispatch(EVT_SCANNED_DEVICES, buffer, length);
}

void Scanner::onBleEvent(ble_evt_t * p_ble_evt) {

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_ADV_REPORT:
		onAdvertisement(&p_ble_evt->evt.gap_evt.params.adv_report);
		break;
	}
}

/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
static uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->data_len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type = p_data[index+1];

        if (field_type == type)
        {
            p_typedata->p_data = &p_data[index+2];
            p_typedata->data_len = field_length-1;
            return NRF_SUCCESS;
        }
        index += field_length+1;
    }
    return NRF_ERROR_NOT_FOUND;
}

bool Scanner::isFiltered(data_t* p_adv_data) {

	//! If we want to send filtered scans once every N times, and now is that time, then just return false
	if (_filterSendFraction > 0 && _scanCount == 0) {
		return false;
	}

	data_t type_data;

	uint32_t err_code = adv_report_parse(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
										 p_adv_data,
										 &type_data);

	if (err_code == NRF_SUCCESS) {
//		_logFirst(INFO, "found manufac data:");
//		BLEutil::printArray(type_data.p_data, type_data.data_len);

		//! [28.01.16] can't cast to uint16_t because it's possible that p_data is not
		//! word aligned!! So have to shift it by hand
		uint16_t companyIdentifier = type_data.p_data[1] << 8 | type_data.p_data[0];
		if (type_data.data_len >= 3 &&
			companyIdentifier == CROWNSTONE_COMPANY_ID) {
//			LOGi("is dobots device!");

//			_logFirst(INFO, "parse data");
//			BLEutil::printArray(type_data.p_data+2, type_data.data_len-2);
//
			DoBotsManufac dobotsManufac;
			dobotsManufac.parse(type_data.p_data+2, type_data.data_len-2);

			switch (dobotsManufac.getDeviceType()) {
			case DEVICE_DOBEACON: {
//				LOGi("found dobeacon");
				return _scanFilter & SCAN_FILTER_DOBEACON_MSK;
			}
			case DEVICE_CROWNSTONE: {
//				LOGi("found crownstone");
				return _scanFilter & SCAN_FILTER_CROWNSTONE_MSK;
			}
			default:
				return false;
			}

		}

	}

	return false;
}

void Scanner::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {

	if (isScanning()) {
		//! we do active scanning, to avoid handling each device twice, only
		//! check the scan responses (as long as we don't care about the
		//! advertisement data)
		if (p_adv_report->scan_rsp) {
            data_t adv_data;

            //! Initialize advertisement report for parsing.
            adv_data.p_data = (uint8_t *)p_adv_report->data;
            adv_data.data_len = p_adv_report->dlen;

			if (!isFiltered(&adv_data)) {
				_scanResult->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
			}
		}
	}
}

void Scanner::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_BLE_EVENT: {
		onBleEvent((ble_evt_t*)p_data);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		_scanDuration = *(uint32_t*)p_data;
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		_scanSendDelay = *(uint32_t*)p_data;
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		_scanBreakDuration = *(uint32_t*)p_data;
		break;
	}
	case CONFIG_SCAN_FILTER: {
		_scanFilter = *(uint8_t*)p_data;
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		_filterSendFraction = *(uint32_t*)p_data;
		break;
	}
	}

}
