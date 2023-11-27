# List of Crownstone application source files.
MESSAGE(STATUS "crownstone source files appended to FOLDER_SOURCE")

# BLE
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_Advertiser.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_BleCentral.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_CharacteristicBase.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_CrownstoneCentral.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_CrownstoneManufacturer.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_iBeacon.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_Service.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_ServiceData.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_Stack.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_UUID.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_Stack.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_Characteristic.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_CrownstoneManufacturer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_ServiceData.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_CharacteristicBase.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_BleCentral.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_Advertiser.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_Service.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_iBeacon.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_CrownstoneCentral.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_NordicMesh.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_Nordic.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_UUID.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/ble/cs_BleConstants.h")

# Common
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/../shared/ipc/cs_IpcRamData.c")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/../shared/ipc/cs_IpcRamDataChecks.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/common/cs_Component.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/common/cs_Handlers.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Tuple.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Handlers.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_BaseClass.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Component.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/common/cs_Types.h")

# Test
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/test/cs_TestCrownstoneCentral.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/test/cs_TestAccess.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/test/cs_Test.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/test/cs_TestCentral.h")

# Drivers
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_ADC.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_COMP.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_GpRegRet.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Watchdog.cpp")

# Encryption
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/encryption/cs_ConnectionEncryption.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/encryption/cs_RC5.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/encryption/cs_ConnectionEncryption.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/encryption/cs_RC5.h")

# Localisation
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_MeshTopology.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_AssetFiltering.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_AssetFilterStore.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_AssetFilterSyncer.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_AssetForwarder.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_AssetStore.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_NearestCrownstoneTracker.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetFilterSyncer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_MeshTopology.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetFilterPacketAccessors.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_TrackableEvent.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetFilterStore.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetForwarder.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_MeshTopologyResearch.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetStore.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetFiltering.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/localisation/cs_AssetRecord.h")

# Logging
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/logging/cs_Logger.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/logging/cs_CLogger.c")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/cs_Logger.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/impl/cs_LogNrf.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/impl/cs_LogNone.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/impl/cs_LogPlainText.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/impl/cs_LogStdPrintf.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/impl/cs_LogUtils.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/impl/cs_LogBinaryProtocol.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/logging/cs_CLogger.h")

# Processing
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_BackgroundAdvHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_CommandAdvHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_CommandHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_ExternalStates.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_FactoryReset.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_MultiSwitchHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_PowerSampling.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_RecognizeSwitch.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_Scanner.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_Setup.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_TapToToggle.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_TemperatureGuard.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_CommandAdvHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_TemperatureGuard.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_FactoryReset.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_CommandHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_PowerSampling.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_BackgroundAdvHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_TapToToggle.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_MultiSwitchHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_RecognizeSwitch.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_Setup.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_ExternalStates.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/processing/cs_Scanner.h")

# Protocol
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/protocol/mesh/cs_MeshModelPacketHelper.cpp")

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

# Services
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/services/cs_CrownstoneService.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/services/cs_DeviceInformationService.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/services/cs_SetupService.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/services/cs_SetupService.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/services/cs_DeviceInformationService.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/services/cs_MicroappService.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/services/cs_CrownstoneService.h")

# Structs
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/structs/buffer/cs_CharacteristicBuffer.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_AssetFilterStructs.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_BufferAccessor.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_BleCentralPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_PacketsInternal.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_StreamBufferAccessor.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_CrownstoneCentralPackets.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_ControlPacketAccessor.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_ResultPacketAccessor.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/cs_CharacteristicStructs.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_CharacteristicBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_CircularBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_CharacteristicReadBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_CharacteristicWriteBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_AdcBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_DifferentialBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_StackBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_EncryptedBuffer.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/structs/buffer/cs_CircularDifferentialBuffer.h")

# Storage
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/storage/cs_StateData.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/storage/cs_IpcRamBluenet.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/storage/cs_IpcRamBluenet.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/storage/cs_State.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/storage/cs_StateData.h")

# Third party
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/third/optmed.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/third/SortMedian.cc")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/third/nrf/app_error_weak.c")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/std/function.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/SortMedian.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/Element.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/nrf/nrf_mesh_config_app.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/nrf/sdk17.1.0/sdk_config.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/nrf/sdk17.1.0/app_config.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/nrf/sdk15.3.0/sdk_config.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/nrf/sdk15.3.0/app_config.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/Median.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/random/seed.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/random/state.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/random/random.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/third/optmed.h")

# Tracking
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/tracking/cs_TrackedDevice.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/tracking/cs_TrackedDevices.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/tracking/cs_TrackedDevices.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/tracking/cs_TrackedDevice.h")

# Uart
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartConnection.cpp")

LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/uart/cs_UartConnection.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/uart/cs_UartHandler.h")
LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/uart/cs_UartCommandHandler.h")

# Util
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_BleError.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_Syscalls.c")

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

IF (BUILD_MICROAPP_SUPPORT)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/microapp/cs_Microapp.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/microapp/cs_MicroappStorage.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/microapp/cs_MicroappController.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/microapp/cs_MicroappRequestHandler.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/microapp/cs_MicroappInterruptHandler.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/microapp/cs_MicroappSdkUtil.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/services/cs_MicroappService.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_DoubleStackCoroutine.c")

	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/microapp/cs_MicroappStorage.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/microapp/cs_MicroappSdkUtil.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/microapp/cs_MicroappRequestHandler.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/microapp/cs_MicroappInterruptHandler.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/microapp/cs_MicroappController.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/microapp/cs_Microapp.h")
ENDIF()

IF (BUILD_TWI)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Twi.cpp")

	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Twi.h")
ENDIF()

IF (BUILD_GPIOTE)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Gpio.cpp")
	
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/drivers/cs_Gpio.h")
ENDIF()

IF (BUILD_MESH_TOPOLOGY_RESEARCH)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_MeshTopologyResearch.cpp")
ENDIF()

IF (BUILD_CLOSEST_CROWNSTONE_TRACKER)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/localisation/cs_NearestCrownstoneTracker.cpp")
ENDIF()

IF (BUILD_MEM_USAGE_TEST)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/test/cs_MemUsageTest.cpp")

	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/test/cs_MemUsageTest.h")
ENDIF()

IF (MESHING AND "${MESHING}" STRGREATER "0" AND BUILD_MESHING AND "${BUILD_MESHING}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "Need to set BUILD_MESHING=1 if MESHING should be enabled!")
ENDIF()

IF (BUILD_MESHING)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_Mesh.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshAdvertiser.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshCommon.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshCore.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshModelMulticast.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshModelMulticastAcked.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshModelMulticastNeighbours.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshModelUnicast.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshModelSelector.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshMsgHandler.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshMsgSender.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshScanner.cpp")
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/mesh/cs_MeshUtil.cpp")

	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshModelMulticast.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshModelUnicast.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshModelMulticastAcked.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshModelMulticastNeighbours.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshModelSelector.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshMsgSender.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshCore.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshCommon.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshDefines.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshScanner.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshMsgHandler.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshUtil.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_Mesh.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshMsgEvent.h")
	LIST(APPEND FOLDER_HEADER "${BLUENET_INCLUDE_DIR}/mesh/cs_MeshAdvertiser.h")

	IF (NOT DEFINED MESH_SDK_DIR)
		MESSAGE(FATAL_ERROR "MESH_SDK_DIR is not defined!")
	ENDIF()

	IF(IS_DIRECTORY "${MESH_SDK_DIR}")
		MESSAGE(STATUS "Use directory ${MESH_SDK_DIR} for the mesh")
	ELSE()
		MESSAGE(FATAL_ERROR "Directory for the mesh, \"${MESH_SDK_DIR}\", does not exist!")
	ENDIF()
ENDIF()