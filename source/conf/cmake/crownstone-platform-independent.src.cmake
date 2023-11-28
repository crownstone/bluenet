######################################################################################################
# If a source file does not strictly depend on the platform to build for, it should be defined here.
######################################################################################################

message(STATUS "crownstone application source files appended to FOLDER_SOURCE")

# Behaviour
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_Behaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_BehaviourConflictResolution.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_BehaviourHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_BehaviourStore.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_ExtendedSwitchBehaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_SwitchBehaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_TwilightBehaviour.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/behaviour/cs_TwilightHandler.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_SwitchBehaviour.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_BehaviourConflictResolution.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_ExtendedSwitchBehaviour.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_TwilightHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_BehaviourMutation.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_BehaviourStore.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_BehaviourHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_Behaviour.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/behaviour/cs_TwilightBehaviour.h")

# Cfg
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/cfg/cs_Boards.c")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_AutoConfig.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_Boards.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_HardwareVersions.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_Debug.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_UuidConfig.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_Strings.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_Git.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_Config.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/cfg/cs_DeviceTypes.h")

# Common
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/common/cs_Component.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/common/cs_Types.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Tuple.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Handlers.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_BaseClass.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Component.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Types.h")

# Drivers
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Timer.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Dimmer.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Timer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Dimmer.h")

# Encryption
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/encryption/cs_AES.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/encryption/cs_KeysAndAccess.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/encryption/cs_AES.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/encryption/cs_KeysAndAccess.h")

# Events
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/events/cs_Event.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/events/cs_EventDispatcher.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/events/cs_EventListener.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/events/cs_EventDispatcher.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/events/cs_EventListener.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/events/cs_Event.h")

# Localisation
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_AssetFilterPacketAccessors.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetFilterPacketAccessors.h")

# Presence
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/presence/cs_PresenceCondition.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/presence/cs_PresenceHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/presence/cs_PresencePredicate.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/presence/cs_PresenceDescription.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/presence/cs_PresenceHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/presence/cs_PresenceCondition.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/presence/cs_PresencePredicate.h")

# Protocol
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/protocol/cs_UartProtocol.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/mesh/cs_MeshModelPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/mesh/cs_MeshModelPacketHelper.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_UartProtocol.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_ServiceDataPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_UartMsgTypes.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_CmdSource.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_MicroappPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_ExactMatchFilterStructs.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_AssetFilterPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_SerialTypes.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_Typedefs.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_CommandTypes.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_CuckooFilterStructs.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_MeshTopologyPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_RssiAndChannel.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_UicrPacket.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_Packets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_UartOpcodes.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/protocol/cs_ErrorCodes.h")

# Storage
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/storage/cs_State.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/storage/cs_StateData.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/storage/cs_IpcRamBluenet.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/storage/cs_State.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/storage/cs_StateData.h")

# Switch
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/switch/cs_SafeSwitch.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/switch/cs_SmartSwitch.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/switch/cs_SwitchAggregator.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/switch/cs_SmartSwitch.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/switch/cs_SafeSwitch.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/switch/cs_SwitchAggregator.h")

# Time
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/time/cs_SystemTime.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/time/cs_TimeOfDay.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/time/cs_TimeSyncMessage.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/time/cs_SystemTime.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/time/cs_Time.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/time/cs_DayOfWeek.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/time/cs_TimeOfDay.h")

# Uart
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartConnection.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartCommandHandler.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/uart/cs_UartConnection.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/uart/cs_UartHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/uart/cs_UartCommandHandler.h")

# Util
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_AssetFilter.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_CuckooFilter.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_ExactMatchFilter.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_WireFormat.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_BitmaskVarSize.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_Hash.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_Crc16.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_Crc32.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_WireFormat.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_BitmaskVarSize.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Hash.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Crc32.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Lollipop.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Store.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_FilterInterface.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_AssetFilter.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Math.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_ExactMatchFilter.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_CuckooFilter.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Crc16.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Coroutine.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Error.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Utils.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_UuidParser.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Syscalls.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_Variance.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_DoubleStackCoroutine.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_RandomGenerator.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/util/cs_BleError.h")
