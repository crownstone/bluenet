######################################################################################################
# If a source file does not strictly depend on the platform to build for, it should be defined here.
######################################################################################################

message(STATUS "crownstone application source files appended to FOLDER_SOURCE")


LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_Behaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_BehaviourConflictResolution.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_BehaviourHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_BehaviourStore.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_ExtendedSwitchBehaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_SwitchBehaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_TwilightBehaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/behaviour/cs_TwilightHandler.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cfg/cs_Boards.c")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/cs_Component.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/cs_Types.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Timer.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/encryption/cs_AES.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_Event.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventDispatcher.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventListener.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_AssetFilterPacketAccessors.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/presence/cs_PresenceCondition.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/presence/cs_PresenceHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/presence/cs_PresencePredicate.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/cs_UartProtocol.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_State.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_StateData.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/switch/cs_SafeSwitch.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/switch/cs_SmartSwitch.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/switch/cs_SwitchAggregator.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/time/cs_SystemTime.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/time/cs_TimeOfDay.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/uart/cs_UartConnection.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/uart/cs_UartHandler.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_AssetFilter.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_CuckooFilter.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_ExactMatchFilter.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_WireFormat.cpp")
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BitmaskVarSize.cpp")
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Hash.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Dimmer.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/uart/cs_UartCommandHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Crc16.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Crc32.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/encryption/cs_KeysAndAccess.cpp")
