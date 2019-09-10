#include "common/cs_Types.h"
#include "events/cs_EventListener.h"

class ScannedDeviceHandler : public EventListener{
    public:
    
    ScannedDeviceHandler();

    // registers with EventDispatcher
    void init();

    virtual void handleEvent(event_t &event);

    private:
    void handleEvtDeviceScanned(TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice);

};