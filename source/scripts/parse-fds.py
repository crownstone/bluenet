#!/usr/bin/python3

# Parses FDS data read from flash.
# Each page starts with: 0xDEADC0DE 0xF11E01FF (swap page) or 0xDEADC0DE 0xF11E01FE (data page)
# FDS record:
#   2B record key
#   2B data length in words
#   2B file ID
#   2B CRC
#   4B record ID
#   data

import re
import sys

if (len(sys.argv) < 2):
	print("Usage: %s <file>" % sys.argv[0])
	exit(1)
fileName = sys.argv[1]

nrfjprog = True

patternData = None
if nrfjprog:
	# Output from nrfjprog: 0x0007DF70: FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF   |................|
	patternData = re.compile("0x([0-9A-F]{8}): (([0-9A-F]{2} ?)+)")
else:
	# Output from JLinkExe, for example: 0006E000 = DE C0 AD DE FE 01 1E F1 00 00 01 00 03 00 C6 60
	patternData = re.compile("([0-9A-F]{8}) = (([0-9A-F]{2} ?)+)")



pageHeaderSize = 8
itemHeaderSize = 12

def getUint16(data, index):
	#print("getUint16 index=", index, "b1=", int(data[index]), "b2=", int(data[index+1]))
	if nrfjprog:
		return (data[index] << 8) + data[index + 1]
	else:
		return data[index] + (data[index + 1] << 8)


def printTypeData(type, fileId, data, sizeLeft):
	for i in range(0, sizeLeft):
		data.append(255)
	if type > 0:
		dataStr = ""
		for b in data:
			dataStr += "%02X " % b
		print("type=%03i fileId=%03i len=%03i data: %s" % (type, fileId, len(data), dataStr))


def parseFile(textFilename):
	file = open(textFilename)
	type = -1
	fileId = -1
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
					sizeLeft = 0

			for i in range(0, 16):
				#print("skip:", skip, " sizeLeft:", sizeLeft)
				if (skip > 0):
					skip -= 1
					if (fileId < 0 and i == 0):
						if (nrfjprog):
							fileId = getUint16(data, i + 2)
						else:
							fileId = getUint16(data, i)
					# go to next byte
					continue
				if (sizeLeft):
					typedata.append(data[i])
					sizeLeft -= 1
				else:
					printTypeData(type, fileId, typedata, sizeLeft)
					# New item
					typeLen = 0
					if (nrfjprog):
						type = getUint16(data, i + 2)
						typeLen = 4 * getUint16(data, i)
						if (i < 16-4):
							fileId = getUint16(data, i + 6)
						else:
							fileId = -1
					else:
						type = getUint16(data, i)
						typeLen = 4 * getUint16(data, i + 2)
						if (i < 16-4):
							fileId = getUint16(data, i + 4)
						else:
							fileId = -1
					if (type == 0xFFFF):
						type = -1
						break
					typedata = []
					sizeLeft = typeLen
					skip = itemHeaderSize - 1
					#print("new item type=", type, "len=", typeLen)
	printTypeData(type, fileId, typedata, sizeLeft)





parseFile(fileName)
