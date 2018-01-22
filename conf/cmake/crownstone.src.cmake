
# essential, code doesn't run without this file !!!
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cs_sysNrf51.c")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cfg/cs_Boards.c")

# Add a single
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Error.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BleError.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Syscalls.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_Settings.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_State.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_StorageHelper.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Storage.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Serial.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_PWM.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_ADC.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_COMP.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_RNG.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_ScanResult.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_TrackDevices.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_ScheduleEntries.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_PowerSamples.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventDispatcher.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_UUID.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Stack.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Service.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Characteristic.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Handlers.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Softdevice.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_CrownstoneManufacturer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_ServiceData.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_EncryptionHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_CommandHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Tracker.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Scanner.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Scheduler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_FactoryReset.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_EnOceanHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_TemperatureGuard.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/cs_UartProtocol.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/SortMedian.cc")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_DeviceInformationService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_CrownstoneService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_SetupService.cpp")
IF(DEFINED INDOOR_SERVICE AND "${INDOOR_SERVICE}" STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_IndoorLocalisationService.cpp")
ENDIF()
IF(GENERAL_SERVICE AND "${GENERAL_SERVICE}" STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_GeneralService.cpp")
ENDIF()
IF(POWER_SERVICE AND "${POWER_SERVICE}" STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_PowerService.cpp")
ENDIF()
IF(SCHEDULE_SERVICE AND "${SCHEDULE_SERVICE}" STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_ScheduleService.cpp")
ENDIF()


# IF(${DEVICE_TYPE} STREQUAL DEVICE_CROWNSTONE_PLUG OR ${DEVICE_TYPE} STREQUAL DEVICE_CROWNSTONE_BUILTIN)
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_PowerSampling.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Switch.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Watchdog.cpp")
# ENDIF()

# should be only when creating iBeacon
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_iBeacon.cpp")

IF(EDDYSTONE AND "${EDDYSTONE}" STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Eddystone.cpp")
ENDIF()

IF(MESHING AND "${MESHING}" STRGREATER "0" AND BUILD_MESHING AND "${BUILD_MESHING}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "Need to set BUILD_MESHING=1 if MESHING should be enabled!")
ENDIF()

IF(DEFINED BUILD_MESHING AND "${BUILD_MESHING}" STRGREATER "0")

	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshControl.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_Mesh.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/mesh/cs_MeshMessageState.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/mesh/cs_MeshMessageCounter.cpp")

	IF(DEFINED MESH_DIR) 
	ELSE() 
		IF(DEFINED MESH_SUBDIR AND DEFINED BLUENET_WORKSPACE_DIR)
			SET(MESH_DIR ${BLUENET_WORKSPACE_DIR}/${MESH_SUBDIR})
		ELSE()
			MESSAGE(FATAL_ERROR "Neither MESH_DIR nor the pair BLUENET_WORKSPACE_DIR/MESH_SUBDIR is defined!")
		ENDIF()
	ENDIF()

	IF(IS_DIRECTORY "${MESH_DIR}")
		MESSAGE(STATUS "Use directory ${MESH_DIR} for the mesh")
	ELSE()
		MESSAGE(FATAL_ERROR "Directory \"${MESH_DIR}\" does not exist! Clone https://github.com/crownstone/nRF51-ble-bcast-mesh and switch to the bluenet branch.")
	ENDIF()

	EXECUTE_PROCESS(
		COMMAND git rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY "${MESH_DIR}"
		OUTPUT_VARIABLE MESH_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	IF(MESH_BRANCH STREQUAL "bluenet")
		MESSAGE(STATUS "Using bluenet branch for the Mesh")
	ELSE()
		MESSAGE(FATAL_ERROR "Switch ${MESH_DIR} to bluenet branch. The master branch is not bluenet-compatible yet.")
	ENDIF()

	SET(INCLUDE_DIR "${INCLUDE_DIR}" "${MESH_DIR}" "${MESH_DIR}/include")

	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/event_handler.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/fifo.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/handle_storage.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/mesh_aci.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/mesh_gatt.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/mesh_packet.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/notification_buffer.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/radio_control.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/rand.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/rbc_mesh.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/timer.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/timeslot.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/timer_scheduler.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/transport_control.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/trickle.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/version_handler.c")

ENDIF()

