#!/usr/bin/env python3
"""
Test the maximum memory usage of the firmware.

Make sure to compile the firmware with BUILD_MEM_USAGE_TEST=1.
Then erase flash and upload all.
Then, run this script.

This script depends on the crownstone python library: https://github.com/crownstone/crownstone-lib-python-uart
"""

import argparse
import logging
import os
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
from bluenet_logs import BluenetLogs

defaultSourceFilesDir = os.path.abspath(f"{os.path.dirname(os.path.abspath(__file__))}/../source")

argParser = argparse.ArgumentParser(description="Client to show binary logs")
argParser.add_argument('--sourceFilesDir',
                       '-s',
                       dest='sourceFilesDir',
                       metavar='path',
                       type=str,
                       default=f"{defaultSourceFilesDir}",
                       help='The path with the bluenet source code files on your system.')
argParser.add_argument('--device',
                       '-d',
                       dest='device',
                       metavar='path',
                       type=str,
                       default=None,
                       help='The UART device to use, for example: /dev/ttyACM0')
args = argParser.parse_args()

sourceFilesDir = args.sourceFilesDir

bluenetLogs = BluenetLogs()
bluenetLogs.setSourceFilesDir(sourceFilesDir)

logFile = sourceFilesDir + "/../mem-usage-test.log"
logging.basicConfig(
	format='%(asctime)s %(levelname)-7s: %(message)s',
	level=logging.INFO,
	handlers=[logging.FileHandler(logFile), logging.StreamHandler()]
)

print("")
print("Make sure you compiled the firmware with:")
print("    CS_SERIAL_ENABLED=SERIAL_ENABLE_RX_AND_TX")
print("    SERIAL_VERBOSITY=SERIAL_BYTE_PROTOCOL_ONLY")
print("    CS_UART_BINARY_PROTOCOL_ENABLED=1")
print("    BUILD_MEM_USAGE_TEST=1")
print("")
print(f"Logging to file: {logFile}")
print("")

# Keep up the operation mode.
stoneHasBeenSetUp = False
def handleHello(data):
	helloResult = data
	logging.log(logging.DEBUG, f"flags={helloResult.status.flags}")
	global stoneHasBeenSetUp
	stoneHasBeenSetUp = helloResult.status.hasBeenSetUp
	logging.log(logging.INFO, f"stoneHasBeenSetUp={stoneHasBeenSetUp}")
UartEventBus.subscribe(UartTopics.hello, handleHello)


# Start up the USB bridge.
uart = CrownstoneUart()
uart.initialize_usb_sync(port=args.device, writeChunkMaxSize=64)

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

stats = {
	"minStackEnd": [],
	"maxHeapEnd": [],
	"minFree": [],
}

def factoryReset() -> bool:
	logging.log(logging.INFO, "Factory reset")
	try:
		controlPacket = ControlPacketsGenerator.getCommandFactoryResetPacket()
		uartMessage = UartMessagePacket(UartTxType.CONTROL, controlPacket).getPacket()
		uartPacket = UartWrapperPacket(UartMessageType.UART_MESSAGE, uartMessage).getPacket()
		result = UartWriter(uartPacket).write_with_result_sync([ResultValue.SUCCESS, ResultValue.WAIT_FOR_SUCCESS])
		if result.resultCode is ResultValue.SUCCESS:
			# This always returns SUCCESS, while we should actually wait.
			time.sleep(10.0)
			return True
		if result.resultCode is ResultValue.WAIT_FOR_SUCCESS:
			# Actually we should wait for the next result code..
			time.sleep(10.0)
			return True
		logging.log(logging.WARN, f"Factory reset failed, result={result}")
		return False

	except CrownstoneException as e:
		logging.log(logging.WARN, f"Failed to factory reset: {e}")
		return False

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

def stop(errStr=None):
	uart.stop()
	if (errStr is None):
		printStats()
		logging.log(logging.INFO, "DONE!")
		exit(0)
	else:
		logging.log(logging.WARN, f"Failed: {errStr}")
		exit(1)

def testRun():
	minStackEnd = 0xFFFFFFFF
	maxHeapEnd = 0x00000000
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
				logging.log(logging.INFO, f"minStackEnd=0x{minStackEnd:08X} maxHeapEnd=0x{maxHeapEnd:08X} minFree={minFree} numSbrkFails={numSbrkFails}")
				stats["minStackEnd"].append(minStackEnd)
				stats["maxHeapEnd"].append(maxHeapEnd)
				stats["minFree"].append(minFree)
				return

			# Keep up the minima and maxima
			minStackEnd = min(ramStats.minStackEnd, minStackEnd)
			maxHeapEnd = max(ramStats.maxHeapEnd, maxHeapEnd)
			minFree = min(ramStats.minFree, minFree)
			numSbrkFails = max(ramStats.numSbrkFails, numSbrkFails)

def printStats():
	arr = stats["maxHeapEnd"]
	logging.log(logging.INFO, f"Stats over {len(arr)} runs:")
	logging.log(logging.INFO, f"maxHeapEnd:  min=0x{min(arr):08X} avg=0x{int(sum(arr) / len(arr)):08X} max=0x{max(arr):08X}")
	arr = stats["minStackEnd"]
	logging.log(logging.INFO, f"minStackEnd: min=0x{min(arr):08X} avg=0x{int(sum(arr)/len(arr)):08X} max=0x{max(arr):08X}")
	arr = stats["minFree"]
	logging.log(logging.INFO, f"minFree:     min={min(arr):10} avg={int(sum(arr) / len(arr)):10} max={max(arr):10}")

def main():
	for i in range(0, 5):
		time.sleep(1)
		if not isSetupMode():
			stop("Not in setup mode")
			return

		success = setup()
		if not success:
			stop("Failed to setup")
		logging.log(logging.INFO, "Setup completed")

		testRun()

		success = factoryReset()
		if not success:
			stop("Failed to factory reset")
		logging.log(logging.INFO, "Factory reset completed")

# Main
try:
	main()
except KeyboardInterrupt:
	pass

stop()
