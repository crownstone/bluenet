#include "processing/cs_ScannedDeviceHandler.h"

#include "cfg/cs_UuidConfig.h"
#include "common/cs_Types.h"
#include "events/cs_EventListener.h"
#include "events/cs_EventDispatcher.h"
#include "processing/cs_EncryptionHandler.h"
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

        // find relevant advertized type
        cs_data_t services16bit;
        if(BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
                scannedDevice->data,
                scannedDevice->dataSize,
                &services16bit) != ErrorCodesGeneral::ERR_SUCCESS){
            LOG("AD_TYPE_SERVICE_DATA not found");
            return;
        }

        //  unpack and validate cs_data_t to service_data_t
        service_data_t* incomingServiceData = nullptr;
        if( ! unpackToServiceData(&services16bit,incomingServiceData)){
            return;
        }

        LOG("protocv %d, devicetype %d",
            incomingServiceData->params.protocolVersion,
            incomingServiceData->params.deviceType);
        
        // decrypt
        if( ! decryptServiceData(incomingServiceData)){
            return;
        }

        // dispatch to subhandlers
        switch(incomingServiceData->params.encrypted.type){
            case  ServiceDataEncryptedType::SERVICE_DATA_TYPE_STATE:{
                handleState(incomingServiceData->params.encrypted.state);    
                break;
            }
            case  ServiceDataEncryptedType::SERVICE_DATA_TYPE_ERROR:{
                handleError(incomingServiceData->params.encrypted.error);
                break;
            }
            case  ServiceDataEncryptedType::SERVICE_DATA_TYPE_EXT_STATE:{
                handleExtState(incomingServiceData->params.encrypted.extState);
                break;
            }
            case  ServiceDataEncryptedType::SERVICE_DATA_TYPE_EXT_ERROR:{
                handleExtError(incomingServiceData->params.encrypted.extError);
                break;
            }
            default:{
                LOG("Unrecognized ServiceDataencryptedType: %d",incomingServiceData->params.encrypted.type);
                return;
            }
        }
}

void ScannedDeviceHandler::handleState(const service_data_encrypted_state_t& state){
    LOG("ScannedDeviceHandler::handleState");
}
void ScannedDeviceHandler::handleError(const service_data_encrypted_error_t& error){
    LOG("ScannedDeviceHandler::handleError");
}
void ScannedDeviceHandler::handleExtState(const service_data_encrypted_ext_state_t& extState){
    LOG("ScannedDeviceHandler::handleExtState");
}
void ScannedDeviceHandler::handleExtError(const service_data_encrypted_ext_error_t& extError){
    LOG("ScannedDeviceHandler::handleExtError");
}


bool ScannedDeviceHandler::unpackToServiceData(cs_data_t* services16bit, service_data_t*& incomingServiceData){
        if(services16bit->len != 2 + sizeof(service_data_t)){
            LOG("ServiceData lenght mismatch, %d != %d",
                services16bit->len, 
                sizeof(service_data_t));
            return false;
        }

        auto incomingAdvDataService_uuid = services16bit->data[1] << 8 | services16bit->data[0];
        if(incomingAdvDataService_uuid != CROWNSTONE_PLUG_SERVICE_DATA_UUID ){
            LOG("SERVICE_DATA_UUID mismatch, %x != %x",incomingAdvDataService_uuid,CROWNSTONE_PLUG_SERVICE_DATA_UUID);
            return false;
        }

        incomingServiceData = reinterpret_cast<service_data_t*>(services16bit->data + 2);
        return true;
}

bool ScannedDeviceHandler::decryptServiceData(service_data_t* incomingServiceData){
    if(incomingServiceData->params.protocolVersion == ServiceDataUnencryptedType::SERVICE_DATA_TYPE_ENCRYPTED){

            auto& EH = EncryptionHandler::getInstance();
            EH._checkAndSetKey(EncryptionAccessLevel::SERVICE_DATA);
            EH.SetCtrNonce_unsafe();
            EH.DecryptCtr(
                incomingServiceData->params.encryptedArray,sizeof(incomingServiceData->params.encryptedArray),
                incomingServiceData->params.encryptedArray,sizeof(incomingServiceData->params.encryptedArray),
                EH._block
            );

            // EncryptionAccessLevel requested_accesslevel;
            // if( ! EncryptionHandler::getInstance().decrypt(
            //     incomingServiceData->params.encryptedArray,             //uint8_t* encryptedDataPacket, 
            //     sizeof(incomingServiceData->params.encryptedArray),     //int16_t encryptedDataPacketLength, 
			// 	incomingServiceData->params.encryptedArray,             //uint8_t* target, 
            //     sizeof(incomingServiceData->params.encryptedArray),     //uint16_t targetLength, 
            //     requested_accesslevel,                   //EncryptionAccessLevel& userLevelInPackage, 
            //     EncryptionType::ECB_GUEST                              //EncryptionType encryptionType = CTR);
            // )){
            //     return false;
            // }

            // if(requested_accesslevel != EncryptionAccessLevel::SERVICE_DATA){
            //     LOG("Unexpected value for RequestedAccessLevel");
            //     return false;
            // }

        } else {
            // what to do with other protocolVersion types?
            LOG("ServiceData decryption for protocolVersion %d unsupported",incomingServiceData->params.protocolVersion);
            return false;
        }

    return true;
}