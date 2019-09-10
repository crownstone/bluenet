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

    bool unpackToServiceData(cs_data_t* services16bit,service_data_t*& incomingServiceData);
    bool decryptServiceData(service_data_t* incomingServiceData);

};