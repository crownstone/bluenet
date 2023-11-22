# List of Crownstone application source files.
MESSAGE(STATUS "crownstone source files appended to FOLDER_SOURCE")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Advertiser.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_BleCentral.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_CharacteristicBase.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_CrownstoneCentral.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_CrownstoneManufacturer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_iBeacon.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Service.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_ServiceData.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Stack.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_UUID.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/../shared/ipc/cs_IpcRamData.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/../shared/ipc/cs_IpcRamDataChecks.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/cs_Component.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/cs_Handlers.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_ADC.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_COMP.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_GpRegRet.cpp")





LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Watchdog.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/encryption/cs_ConnectionEncryption.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/encryption/cs_RC5.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_MeshTopologyResearch.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_AssetFiltering.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_AssetFilterStore.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_AssetFilterSyncer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_AssetForwarder.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_AssetStore.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/logging/cs_Logger.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/logging/cs_CLogger.c")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_BackgroundAdvHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_CommandAdvHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_CommandHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_ExternalStates.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_FactoryReset.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_MultiSwitchHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_PowerSampling.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_RecognizeSwitch.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Scanner.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Setup.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_TapToToggle.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_TemperatureGuard.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/mesh/cs_MeshModelPacketHelper.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_CrownstoneService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_DeviceInformationService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_SetupService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/buffer/cs_CharacteristicBuffer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_StateData.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_IpcRamBluenet.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/optmed.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/SortMedian.cc")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/nrf/app_error_weak.c")


LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/tracking/cs_TrackedDevice.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/tracking/cs_TrackedDevices.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/uart/cs_UartConnection.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BleError.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Syscalls.c")


IF (BUILD_MICROAPP_SUPPORT)
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/microapp/cs_Microapp.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/microapp/cs_MicroappStorage.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/microapp/cs_MicroappController.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/microapp/cs_MicroappRequestHandler.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/microapp/cs_MicroappInterruptHandler.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/microapp/cs_MicroappSdkUtil.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_MicroappService.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_DoubleStackCoroutine.c")
ENDIF()

IF (BUILD_TWI)
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Twi.cpp")
ENDIF()

IF (BUILD_GPIOTE)
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Gpio.cpp")
ENDIF()

IF (BUILD_MESH_TOPOLOGY_RESEARCH)
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_MeshTopologyResearch.cpp")
ENDIF()

IF (BUILD_CLOSEST_CROWNSTONE_TRACKER)
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/localisation/cs_NearestCrownstoneTracker.cpp")
ENDIF()

IF (BUILD_MEM_USAGE_TEST)
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/test/cs_MemUsageTest.cpp")
ENDIF()

IF (MESHING AND "${MESHING}" STRGREATER "0" AND BUILD_MESHING AND "${BUILD_MESHING}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "Need to set BUILD_MESHING=1 if MESHING should be enabled!")
ENDIF()

IF (BUILD_MESHING)
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_Mesh.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshAdvertiser.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshCommon.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshCore.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshModelMulticast.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshModelMulticastAcked.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshModelMulticastNeighbours.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshModelUnicast.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshModelSelector.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshMsgHandler.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshMsgSender.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshScanner.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshUtil.cpp")

	IF (NOT DEFINED MESH_SDK_DIR)
		MESSAGE(FATAL_ERROR "MESH_SDK_DIR is not defined!")
	ENDIF()

	IF(IS_DIRECTORY "${MESH_SDK_DIR}")
		MESSAGE(STATUS "Use directory ${MESH_SDK_DIR} for the mesh")
	ELSE()
		MESSAGE(FATAL_ERROR "Directory for the mesh, \"${MESH_SDK_DIR}\", does not exist!")
	ENDIF()
ENDIF()
