#!/usr/bin/env python3
"""
Test the maximum memory usage of the firmware.

Make sure to compile the firmware with BUILD_MEM_USAGE_TEST=1.
Then erase flash and upload all.
Then, run this script.

This script depends on the crownstone python library: https://github.com/crownstone/crownstone-lib-python-uart
"""

import logging
import time

from crownstone_core import Conversion
from crownstone_core.Exceptions import CrownstoneException
from crownstone_core.packets.debug.RamStatsPacket import RamStatsPacket
from crownstone_core.protocol.BlePackets import ControlStateGetPacket, ControlStateGetResultPacket, ControlPacket
from crownstone_core.protocol.BluenetTypes import StateType, ResultValue, ControlType
from crownstone_core.protocol.ControlPackets import ControlPacketsGenerator
from crownstone_uart import CrownstoneUart, UartEventBus, UartTopics
from crownstone_uart.core.dataFlowManagers.UartWriter import UartWriter
from crownstone_uart.core.uart.UartTypes import UartMessageType, UartTxType
from crownstone_uart.core.uart.uartPackets.UartCrownstoneHelloPacket import UartCrownstoneHelloPacket
from crownstone_uart.core.uart.uartPackets.UartMessagePacket import UartMessagePacket
from crownstone_uart.core.uart.uartPackets.UartWrapperPacket import UartWrapperPacket
from crownstone_uart.topics.DevTopics import DevTopics

#logging.basicConfig(format='%(levelname)-7s: %(message)s', level=logging.DEBUG)
logging.basicConfig(format='%(levelname)-7s: %(message)s', level=logging.INFO)

uart = CrownstoneUart()


# Keep up the operation mode.
stoneHasBeenSetUp = False
def handleHello(data):
	helloResult = data
	logging.log(logging.DEBUG, f"flags={helloResult.status.flags}")
	global stoneHasBeenSetUp
	stoneHasBeenSetUp = helloResult.status.hasBeenSetUp
	logging.log(logging.DEBUG, f"stoneHasBeenSetUp={stoneHasBeenSetUp}")
UartEventBus.subscribe(UartTopics.hello, handleHello)


# Start up the USB bridge.
uart.initialize_usb_sync(writeChunkMaxSize=64)

# Sphere specific settings:
adminKey = "adminKeyForCrown"
memberKey = "memberKeyForHome"
basicKey = "basicKeyForOther"
serviceDataKey = "MyServiceDataKey"
localizationKey = "aLocalizationKey"
meshAppKey = "MyGoodMeshAppKey"
meshNetworkKey = "MyGoodMeshNetKey"
ibeaconUUID = "1843423e-e175-4af0-a2e4-31e32f729a8a"
sphereId = 1

# Stone specific settings:
crownstoneId = 200
meshDeviceKey = "aStoneKeyForMesh"
ibeaconMajor = 123
ibeaconMinor = 456

def setup() -> bool:
	logging.log(logging.INFO, "Perform setup")
	try:
		controlPacket = ControlPacketsGenerator.getSetupPacket(
			crownstoneId=crownstoneId,
			sphereId=sphereId,
			adminKey=Conversion.ascii_or_hex_string_to_16_byte_array(adminKey),
			memberKey=Conversion.ascii_or_hex_string_to_16_byte_array(memberKey),
			basicKey=Conversion.ascii_or_hex_string_to_16_byte_array(basicKey),
			serviceDataKey=Conversion.ascii_or_hex_string_to_16_byte_array(serviceDataKey),
			localizationKey=Conversion.ascii_or_hex_string_to_16_byte_array(localizationKey),
			meshDeviceKey=Conversion.ascii_or_hex_string_to_16_byte_array(meshDeviceKey),
			meshAppKey=Conversion.ascii_or_hex_string_to_16_byte_array(meshAppKey),
			meshNetworkKey=Conversion.ascii_or_hex_string_to_16_byte_array(meshNetworkKey),
			ibeaconUUID=ibeaconUUID,
			ibeaconMajor=ibeaconMajor,
			ibeaconMinor=ibeaconMinor
		)
		uartMessage = UartMessagePacket(UartTxType.CONTROL, controlPacket).getPacket()
		uartPacket = UartWrapperPacket(UartMessageType.UART_MESSAGE, uartMessage).getPacket()
		result = UartWriter(uartPacket).write_with_result_sync([ResultValue.SUCCESS, ResultValue.WAIT_FOR_SUCCESS])
		if result.resultCode is ResultValue.SUCCESS:
			return True
		if result.resultCode is ResultValue.WAIT_FOR_SUCCESS:
			# Actually we should wait for the next result code..
			time.sleep(3.0)
			return True
		logging.log(logging.WARN, f"Setup failed, result={result}")
		return False

	except CrownstoneException as e:
		logging.log(logging.WARN, f"Failed to setup: {e}")
		return False

def isSetupMode() -> bool:
	return stoneHasBeenSetUp is False

def getRamStats():
	controlPacket = ControlPacket(ControlType.GET_RAM_STATS).getPacket()
	uartMessage = UartMessagePacket(UartTxType.CONTROL, controlPacket).getPacket()
	uartPacket = UartWrapperPacket(UartMessageType.UART_MESSAGE, uartMessage).getPacket()
	try:
		result = UartWriter(uartPacket).write_with_result_sync()
		if result.resultCode is ResultValue.SUCCESS:
			return RamStatsPacket(result.payload)
		logging.log(logging.WARN, f"Get ram stats failed, result={result}")
	except CrownstoneException as e:
		logging.log(logging.WARN, f"Get ram stats failed: {e}")
	return None

# Main
def stop():
	logging.log(logging.INFO, f"minStackEnd=0x{minStackEnd:08X} maxHeapEnd=0x{maxHeapEnd:08X} minFree={minFree} numSbrkFails={numSbrkFails}")
	uart.stop()
	exit(0)

try:
	time.sleep(1)
	if isSetupMode():
		success = setup()
		if not success:
			exit(1)
		logging.log(logging.INFO, "Setup completed")
		time.sleep(2)

#	uart._usbDev.resetCrownstone()
#	time.sleep(3)

	minStackEnd = 0xFFFFFFFF
	maxHeapEnd =  0x00000000
	minFree = minStackEnd - maxHeapEnd
	numSbrkFails = 0
	similarStats = 0

	while True:
		time.sleep(1)
		ramStats = getRamStats()
		if ramStats is not None:
			logging.log(logging.INFO, ramStats)

			if (minStackEnd == ramStats.minStackEnd) and (maxHeapEnd == ramStats.maxHeapEnd):
				similarStats += 1
			else:
				similarStats = 0
			if similarStats > 60:
				logging.log(logging.INFO, "No change in RAM statistics for some time.")
				stop()

			# Keep up the minima and maxima
			minStackEnd = min(ramStats.minStackEnd, minStackEnd)
			maxHeapEnd = max(ramStats.maxHeapEnd, maxHeapEnd)
			minFree = min(ramStats.minFree, minFree)
			numSbrkFails = max(ramStats.numSbrkFails, numSbrkFails)


except KeyboardInterrupt:
	pass

stop()
