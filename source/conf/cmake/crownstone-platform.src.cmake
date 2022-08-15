######################################################################################################
# If a source file strictly depends on platform headers etc., it should be defined here.
######################################################################################################

message(STATUS "crownstone platform dependent source files appended to FOLDER_SOURCE")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Uicr.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_State.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_RNG.cpp")