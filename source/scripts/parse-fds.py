#!/usr/bin/python3

# Parses FDS data read from flash.

import re
import sys

if (len(sys.argv) < 2):
	print("Usage: %s <file>" % sys.argv[0])
	exit(1)
fileName = sys.argv[1]

# Output from JLinkExe, for example: 0006E000 = DE C0 AD DE FE 01 1E F1 00 00 01 00 03 00 C6 60
patternData = re.compile("([0-9A-F]{8}) = (([0-9A-F]{2} ?)+)")

pageHeaderSize = 8
itemHeaderSize = 12

def getUint16(data, index):
	#print("getUint16 index=", index, "b1=", int(data[index]), "b2=", int(data[index+1]))
	return data[index] + (data[index + 1] << 8)

def printTypeData(type, data):
	if type >= 0:
		dataStr = ""
		for b in data:
			dataStr += "%02X " % b
		print("type=%3i len=%3i data: %s" % (type, len(data), dataStr))


def parseFile(textFilename):
	file = open(textFilename)
	type = -1
	typedata = []
	sizeLeft = 0
	skip = 0
	for line in file:
		#print("line:", line)
		match = patternData.findall(line)
		if (len(match)):
			#print("found line:", line)
			#print("match=", match, "len=", len(match))
			dataAddressStr = match[0][0]
			dataAddress = int(dataAddressStr, 16)
			#dataStr = match[0][1].replace(" ", "")
			dataStr = match[0][1]
			#print("addr=", dataAddressStr, "data=", dataStr)
			data = bytes.fromhex(dataStr)
			#print("dataAddress=%8X" % dataAddress)
			#print("len=", len(data), "bytes=", data)

			newPage = False
			newPageEmpty = True
			index = 0
			if (dataAddress % 0x1000 == 0):
				newPage = True
				for i in range(8, len(data)):
					#print("i=", i, "val=", data[i])
					if (data[i] != 0xFF):
						newPageEmpty = False
						break

			if newPage:
				if newPageEmpty:
					print("empty page")
					skip = 0
					sizeLeft = 0
					# go to next line
					continue
				else:
					print("new page")
					skip = pageHeaderSize

			for i in range(0, 16):
				if (skip > 0):
					skip -= 1
					# go to next byte
					continue
				if (sizeLeft):
					typedata.append(data[i])
					sizeLeft -= 1
				else:
					printTypeData(type, typedata)
					# New item
					type = getUint16(data, i)
					if (type == 0xFFFF):
						type = -1
						break
					typeLen = 4 * getUint16(data, i + 2)
					typedata = []
					sizeLeft = typeLen
					skip = itemHeaderSize - 1
					#print("new item type=", type, "len=", typeLen)
	printTypeData(type, typedata)





parseFile(fileName)
