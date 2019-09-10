#include "processing/cs_ScannedDeviceHandler.h"

#include "cfg/cs_UuidConfig.h"
#include "common/cs_Types.h"
#include "events/cs_EventListener.h"
#include "events/cs_EventDispatcher.h"
#include "protocol/cs_ServiceDataPackets.h"
#include "util/cs_Error.h"
#include "util/cs_Utils.h"

#include "drivers/cs_Serial.h"

#include <app_util.h>

#define LOG(...) do {LOGd(__VA_ARGS__)} while(0)

ScannedDeviceHandler::ScannedDeviceHandler():
    EventListener(){
}

void ScannedDeviceHandler::init(){
    EventDispatcher::getInstance().addListener(this);
}

void ScannedDeviceHandler::handleEvent(event_t & event){
    LOG("ScannerDeviceHandler::handleEvent: %d", event.type);

    switch(event.type){
        case CS_TYPE::EVT_DEVICE_SCANNED:{
            LOG(" ** EVT_DEVICE_SCANNED ** ");
            assert(event.data, "nullptr exception: scanned device data");

            handleEvtDeviceScanned(static_cast<TYPIFY(EVT_DEVICE_SCANNED) *>(event.data));
            break;
        }

        default:{
            break;
        }
    }
}

void ScannedDeviceHandler::handleEvtDeviceScanned(TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice){
     BLEutil::printAdvTypes(scannedDevice->data,scannedDevice->dataSize);

        cs_data_t services16bit;
        if(BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
                scannedDevice->data,
                scannedDevice->dataSize,
                &services16bit) != ErrorCodesGeneral::ERR_SUCCESS){
            LOG("AD_TYPE_SERVICE_DATA not found");
            return;
        }

        // ------------

        if(services16bit.len != 2 + sizeof(service_data_t)){
            LOG("ServiceData lenght mismatch, %d != %d",
                services16bit.len, 
                sizeof(service_data_t));
            return;
        }

        auto incomingAdvDataService_uuid = services16bit.data[1] << 8 | services16bit.data[0];

        if(incomingAdvDataService_uuid != CROWNSTONE_PLUG_SERVICE_DATA_UUID ){
            LOG("SERVICE_DATA_UUID mismatch, %x != %x",incomingAdvDataService_uuid,CROWNSTONE_PLUG_SERVICE_DATA_UUID);
            return;
        }

        auto incomingServiceData = reinterpret_cast<service_data_t*>(services16bit.data + 2);
        LOG("protocv %d, devicetype %d",incomingServiceData->params.protocolVersion,incomingServiceData->params.deviceType);
        
        (void)incomingServiceData;
}