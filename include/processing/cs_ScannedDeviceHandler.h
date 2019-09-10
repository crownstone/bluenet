#include "common/cs_Types.h"
#include "events/cs_EventListener.h"
#include "protocol/cs_ServiceDataPackets.h"
#include "util/cs_Utils.h"

class ScannedDeviceHandler : public EventListener{
    public:
    
    ScannedDeviceHandler();

    // registers with EventDispatcher
    void init();

    virtual void handleEvent(event_t &event);

    private:
    void handleEvtDeviceScanned(TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice);

    void handleState(const service_data_encrypted_state_t& state);
    void handleError(const service_data_encrypted_error_t& error);
    void handleExtState(const service_data_encrypted_ext_state_t& extState);
    void handleExtError(const service_data_encrypted_ext_error_t& extError);

    bool unpackToServiceData(cs_data_t* services16bit,service_data_t*& incomingServiceData);
    bool decryptServiceData(service_data_t* incomingServiceData);

};