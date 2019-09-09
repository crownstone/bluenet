#include "events/cs_EventListener.h"

class ScannedDeviceHandler : public EventListener{
    public:
    
    ScannedDeviceHandler();

    // registers with EventDispatcher
    void init();

    virtual void handleEvent(event_t &event);

};