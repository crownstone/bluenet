######################################################################################################
# If a source file strictly depends on platform headers etc., it should be defined here.
######################################################################################################

message(STATUS "crownstone platform dependent source files appended to FOLDER_SOURCE")

# Drivers
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_PWM.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Relay.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_RNG.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_RTC.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Serial.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Storage.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Uicr.c")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/storage/cs_State.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_RTC.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Timer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Uicr.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_COMP.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Watchdog.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Serial.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_GpRegRet.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Temperature.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_ADC.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Relay.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Dimmer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_RNG.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Storage.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_PWM.h")