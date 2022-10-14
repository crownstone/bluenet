######################################################################################################
# If a source file strictly depends on platform headers etc., it should be defined here.
######################################################################################################

message(STATUS "crownstone platform dependent source files appended to FOLDER_SOURCE")


LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_PWM.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_RNG.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_RTC.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Serial.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Storage.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Uicr.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_State.cpp")
