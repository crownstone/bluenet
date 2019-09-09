#include "processing/cs_ScannedDeviceHandler.h"

#include "events/cs_EventListener.h"
#include "events/cs_EventDispatcher.h"

#include "drivers/cs_Serial.h"

#define LOG(...) do {LOGd(__VA_ARGS__)} while(0)

ScannedDeviceHandler::ScannedDeviceHandler():
    EventListener(){
}

void ScannedDeviceHandler::init(){
    EventDispatcher::getInstance().addListener(this);
}

void ScannedDeviceHandler::handleEvent(event_t & event){
    // TODO
    LOG("ScannerDeviceHandler::handleEvent: %d", event.type);
}