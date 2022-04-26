#!/usr/bin/env python3

"""
Client to show binary logs.
"""
import time
import argparse
import os

from crownstone_uart import CrownstoneUart
from crownstone_uart.topics.SystemTopics import SystemTopics
from crownstone_uart.core.UartEventBus import UartEventBus

from bluenet_logs import BluenetLogs

import logging

defaultLogStringsFile = os.path.abspath(f"{os.path.dirname(os.path.abspath(__file__))}/../build/default/extracted_logs.json")

argParser = argparse.ArgumentParser(description="Client to show binary logs")
argParser.add_argument('--logStringsFile',
                       '-l',
                       dest='logStringsFileName',
                       metavar='path',
                       type=str,
                       default=f"{defaultLogStringsFile}",
                       help='The path of the file with the extracted logs on your system.')
argParser.add_argument('--device',
                       '-d',
                       dest='device',
                       metavar='path',
                       type=str,
                       default=None,
                       help='The UART device to use, for example: /dev/ttyACM0')
argParser.add_argument('--verbose',
                       '-v',
                       dest="verbose",
                       action='store_true',
                       help='Show verbose output')
argParser.add_argument('--raw',
                       '-r',
                       dest="raw",
                       action='store_true',
                       help='Show raw output (may result in interleaved print statements)')
args = argParser.parse_args()

if args.verbose:
	logging.basicConfig(format='%(asctime)s %(levelname)-7s: %(message)s', level=logging.DEBUG)

logStringsFileName = args.logStringsFileName

print(f"Listening for logs on port {args.device}, and using \"{logStringsFileName}\" to find the log formats.")

# Init bluenet logs, it will listen to events from the Crownstone lib.
bluenetLogs = BluenetLogs()


# Set the dir containing the bluenet source code files.
bluenetLogs.setLogStringsFile(logStringsFileName)


# stub raw message printer that hooks into the uart event bus and prints immediately.
class RawMessagePrinter():
    def __init__(self):
        try:
            UartEventBus.subscribe(SystemTopics.uartRawData, self.onDataReceived)
            print("printing raw data enabled")
        except AttributeError as e:
            print("Failed enabling raw data printer. Are your crownstone python libs up to date?")

    def onDataReceived(self, dat):
        print(dat.decode('utf-8'), end='', flush=True)

if args.raw:
    
    rawprinter = RawMessagePrinter()



# Init the Crownstone UART lib.
uart = CrownstoneUart()
uart.initialize_usb_sync(port=args.device)

# The try except part is just to catch a control+c to gracefully stop the UART lib.
try:
	# Simply keep the program running.
	while True:
		time.sleep(1)
except KeyboardInterrupt:
	pass
finally:
	print("\nStopping UART..")
	uart.stop()
	print("Stopped")
