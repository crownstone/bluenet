#from pylab import *
import matplotlib.pyplot as plt
import sys
import time
import datetime
import re
import os
import cPickle as pickle
import json
import operator

"""
Parses a (timestamped!) minicom capture and plots graphs
At the bottom, select what to plot
"""

jawBones = {
	"C5:4F:A0:00:20:A6":"1",
	"D6:49:EB:45:48:9B":"2",
	"D3:41:EE:0F:5F:DC":"3",
	"EE:E4:04:45:35:1C":"4",
	"D9:30:DD:8B:08:07":"5",
	"EF:5B:87:42:7A:A8":"6",
	"EC:9B:F8:75:5E:BA":"7",
	"F6:1F:00:EE:76:F2":"8",
	"D8:81:E2:53:8A:7A":"9",
	"C7:3D:B1:AB:43:8D":"10",
	"C9:B1:1F:DD:9F:FF":"11",
	"CB:18:6F:55:04:4C":"12",
	"DE:3F:87:54:32:EF":"13",
	"E7:6E:56:4D:B0:12":"14",
	"F2:E4:3A:F3:EB:63":"15",
	"F4:A9:86:17:16:81":"16",
	"EE:F9:B5:A5:26:6F":"17",
	"D2:40:6A:F8:ED:1B":"18",
	"ED:EC:18:BF:BB:22":"19",
	"D6:F4:3D:F9:41:7E":"20",
	"F8:D8:C1:27:DE:A0":"21",
	"EC:5C:CC:F6:CF:B6":"22",
	"DD:87:41:12:D2:61":"23",
	"E9:7A:E6:D7:60:12":"24",
	"FE:AD:80:17:81:32":"25",
	"C3:A2:18:F9:FC:D9":"26",
	"F1:F2:5A:C5:B3:86":"27",
	"D1:29:15:DF:17:CD":"28",
	"F6:5F:FC:B5:AC:4A":"29",
	"D4:56:1C:38:30:D5":"30",
	"DF:17:E6:41:65:83":"31",
	"E5:A9:B2:CA:7B:7F":"32",
	"D8:07:BC:E7:75:03":"33",
	"DA:BD:B2:3A:CF:A8":"34",
	"D2:E3:0B:F6:F1:21":"35",
	"E5:8A:16:7D:83:B3":"36",
	"ED:8B:F9:3B:0E:B6":"37",
	"D8:9D:6E:77:2D:A8":"38",
	"CE:F1:97:31:DB:51":"39",
	"EF:59:29:1B:D3:91":"40",
	"CC:C5:87:95:5B:18":"41",
	"CB:44:67:84:81:F2":"42",
	"D7:86:64:1E:EE:9D":"43",
	"CD:A0:F7:6A:D2:82":"44",
	"D6:85:E3:9E:C5:1A":"45",
	"D6:20:51:29:D8:69":"46",
}

beaconNames = {
	"E3:BF:7F:5F:59:0C":"kontakt 1",
	"DF:8F:3F:A7:A0:E":"kontakt 2",
	"C0:B7:12:98:FD:C":"kontakt 3",

	"E6:EF:B4:98:25:4B":"usb14",
	"E5:6F:56:1F:97:83":"usb54",
	"C4:D0:90:6F:53:4B":"usb56",
	"D0:E8:42:71:C2:5D":"usb76",
	"D0:6D:AA:45:54:55":"usb91",

	"F8:27:73:28:DA:FE":"hans",
	"F1:C1:B8:AC:03:CD":"chloe",
	"E1:89:95:C1:06:04":"meeting",
	"DB:26:1F:D9:FA:5E":"old office chloe",
	"F0:20:A1:2C:57:D4":"hallway 0",
	"C9:92:1A:78:F4:81":"trash room",
	"CF:5E:84:EF:00:91":"lunch room",
	"ED:F5:F8:E3:6A:F6":"kitchen",
	"EB:4D:30:14:6D:C1":"server room",
	"F4:A2:89:23:53:92":"basement",
	"EB:82:34:DA:EE:0B":"balcony",
	"ED:AF:F3:7E:E1:47":"jan geert",
	"C6:27:A8:D7:D4:C7":"allert",
	"E0:31:D7:C5:CA:FF":"hallway 1",
	"D7:59:D6:BD:2A:5A":"dobots software",
	"DE:41:8E:2F:58:85":"interns",
	"D7:D5:51:82:49:43":"peet",
	"FD:CB:99:58:0B:88":"hallway 2",
	"EF:36:60:78:1F:1D":"dobots hardware",
	"E8:B7:37:29:F4:77":"hallway 3",
	"C5:25:3F:5E:92:6F":"small multi purpose room",
	"EA:A6:ED:8A:13:8E":"billiard table",
	"C5:71:64:3A:15:74":"proto table",

	"DC:1A:5A:AF:1A:44":"hans",
	"C6:A0:0C:5A:C8:C6":"chloe",
	"D4:0B:9E:30:B2:73":"meeting",
	"CC:02:60:7A:83:46":"old office chloe",
	"D5:A7:34:EC:72:90":"hallway 0",
	"DB:E3:E7:9B:60:81":"trash room",
	"E8:DF:9A:90:54:6C":"beacon 3",
	"C4:D9:1C:34:16:09":"beacon 4",
	"C0:82:3E:B9:F5:91":"server room",
	"D7:BE:C6:21:71:F2":"basement",
	"EB:C9:C2:58:52:C4":"balcony",
	"C2:92:09:5F:04:78":"jan geert",
	"F8:85:28:C1:4D:D2":"beacon 10",
	"C6:B1:9A:0A:25:E0":"beacon 1",
	"F8:AC:E5:C4:E0:3E":"beacon 8",
	"FA:23:55:F1:04:0E":"beacon 7",
	"F5:A7:4B:49:8C:7D":"beacon 5",
	"C0:6E:12:A4:6B:06":"beacon 6",
	"EF:79:08:EF:50:AC":"dobots hardware",
	"E8:00:93:4E:7B:D9":"beacon 0",
	"E8:C5:AE:A7:6B:A9":"small multi purpose room",
	"EA:64:68:47:B8:EE":"beacon 2",
	"FE:04:85:F9:8F:E9":"beacon 9",
	"EC:9C:70:56:9F:90":"32k",
}


def parseMinicom(filename):
	"""
	Parses a log file and returns the parsed data
	:param filename:
	:return:
	data["scans"]
		scans["node address"] = [entry, entry, ...]
			entry["time"] = timestamp
			entry["address"] = address
			entry["rssi"] = rssi
			entry["occurances"] = occurances
	data["startTimestamp"] = first seen timestamp
	data["endTimestamp"] = last seen timestamp
	"""
	scanning = False
	logfile = open(filename, "r")

	# Search for something like:
	# [2015-12-24 11:23:16] [cs_MeshControl.cpp             : 68   ] Device D0 E8 42 71 C2 5D scanned these devices:
	startPattern = re.compile("\\[([0-9 \\-:]+)\\] .* Device ([0-9A-F ]+) scanned these devices:")

	# Search for something like:
	# [2015-12-24 11:23:16] [cs_MeshControl.cpp             : 79   ] 2: [C4 D0 90 6F 53 4B]   rssi:  -51    occ:  10
	scanPattern = re.compile("\\[([0-9 \\-:]+)\\] .* [0-9]+: \\[([0-9A-F ]+)\\]\\s+rssi:\\s+(-?[0-9]+)\\s+occ:\\s+([0-9]+)")

	# scans: map with:
	#	scans["node address"] = [entry, entry, ...]
	#		entry["time"] = timestamp
	#		entry["address"] = address
	#		entry["rssi"] = rssi
	#		entry["occurances"] = occurances
	#	scans["startTimestamp"] = first seen timestamp
	#	scans["endTimestamp"] = last seen timestamp

	nodes = []
	scans = {}
	data = {"scans" : scans}

	startFound = False
	startTimestamp = -1
	endTimestamp = -1
	address = ""
	for line in logfile:
		matches = startPattern.findall(line)
		if len(matches):
			timestamp = time.mktime(datetime.datetime.strptime(matches[0][0], "%Y-%m-%d %H:%M:%S").timetuple())
			if (startTimestamp < 0):
				startTimestamp = timestamp
			address = matches[0][1].replace(" ", ":")
			if (address not in nodes):
				nodes.append(address)

			startFound = True
#			print matches

		if (startFound):
			matches = scanPattern.findall(line)
			if len(matches):
#				startFound = False
				timestamp = time.mktime(datetime.datetime.strptime(matches[0][0], "%Y-%m-%d %H:%M:%S").timetuple())
				endTimestamp = timestamp
				entry = {"time":timestamp, "address":(matches[0][1]).replace(" ", ":"), "rssi":matches[0][2], "occurances":matches[0][3]}
				if (address not in scans):
					scans[address] = [entry]
				else:
					scans[address].append(entry)
#				print matches

	logfile.close()

#	print nodes
	print "Nodes:"
	for addr in nodes:
		scannerName = addr
		if (addr in beaconNames):
			scannerName = beaconNames[addr]
		print scannerName

#	print scans
	data["startTimestamp"] = startTimestamp
	data["endTimestamp"] = endTimestamp
	return data




def parseHubData(filename):
	"""
	Parses a log file and returns the parsed data
	:param filename:
	:return:
	data["scans"]
		scans["node address"] = [entry, entry, ...]
			entry["time"] = timestamp
			entry["address"] = address
			entry["rssi"] = rssi
	data["startTimestamp"] = first seen timestamp
	data["endTimestamp"] = last seen timestamp
	"""
	logfile = open(filename, "r")
	scans = {}
	data = {"scans" : scans}
	startTimestamp = -1
	endTimestamp = -1
	for line in logfile:
		try:
			jscan = json.loads(line)
		except:
			print "json error at line:"
			print line
			exit()

		timestamp = time.mktime(datetime.datetime.strptime(jscan["timestamp"], "%Y-%m-%dT%H:%M:%S").timetuple())
		if (startTimestamp < 0):
			startTimestamp = timestamp
		endTimestamp = timestamp
		scannedDevices = jscan["scannedDevices"]
#		nodeAddress = "00:00:00:00:00:00"
		nodeAddress = jscan["source_address"]

		for dev in scannedDevices:

			scannedAddress = dev["address"]
			rssi = dev["rssi"]
			entry = {"time":timestamp, "address":scannedAddress, "rssi":rssi}
#			if (entry["address"] not in jawBones):
#				continue
			if (nodeAddress not in data["scans"]):
				data["scans"][nodeAddress] = [entry]
			else:
				data["scans"][nodeAddress].append(entry)
	logfile.close()
	data["startTimestamp"] = startTimestamp
	data["endTimestamp"] = endTimestamp
	for node in scans.keys():
		print node

	return data


def parseRssiTest(filename):
	"""
	Parses a log file and returns the parsed data
	:param filename:
	:return:
	data["scans"]
		scans["node address"] = [entry, entry, ...]
			entry["time"] = timestamp
			entry["address"] = address
			entry["rssi"] = rssi
	data["startTimestamp"] = first seen timestamp
	data["endTimestamp"] = last seen timestamp
	"""
	logfile = open(filename, "r")
	scans = {}
	data = {"scans" : scans}
	startTimestamp = -1
	endTimestamp = -1

	nodeAddress = "00:00:00:00:00:00"
	scans[nodeAddress] = []

	for line in logfile:
		items = line.rstrip().split(",")
		if (len(items) is not 3):
			continue

		timestamp = time.mktime(datetime.datetime.strptime(items[0], "%Y-%m-%dT%H:%M:%S").timetuple())
		address = items[1]
		rssi = items[2]
		entry = {"time":timestamp, "address":address, "rssi":rssi}
		scans[nodeAddress].append(entry)

		if (startTimestamp < 0):
			startTimestamp = timestamp
		endTimestamp = timestamp
	logfile.close()
	data["startTimestamp"] = startTimestamp
	data["endTimestamp"] = endTimestamp
	return data


def filterDevAddresses(data, addresses, remove=False):
	scans = data["scans"]
	for addr in scans:
		for i in range(len(scans[addr])-1, -1,-1):
			scan = scans[addr][i]
			dev = scan["address"]
			if (dev in addresses and remove):
				scans[addr].pop(i)
			if (dev not in addresses and not remove):
				scans[addr].pop(i)
	return data

def filterMostScannedDevices(data, minNumTimesScanned):
	numTimesScanned = {}
	scans = data["scans"]
	for addr in scans:
		for scan in scans[addr]:
			dev = scan["address"]
			if (dev not in numTimesScanned):
				numTimesScanned[dev] = 1
			else:
				numTimesScanned[dev] += 1

#	numTimesScannedList = []
#	for dev in numTimesScanned:
#		numTimesScannedList.append({"address": dev, "num": numTimesScanned[dev]})
#	sortedList = sorted(numTimesScannedList, key=lambda dev: dev["num"], reverse=True)
#	sortedList = sorted(numTimesScannedList, key=operator.attrgetter("num"), reverse=True)

	for addr in scans:
		for i in range(len(scans[addr])-1, -1,-1):
			scan = scans[addr][i]
			dev = scan["address"]
			if (numTimesScanned[dev] < minNumTimesScanned):
				scans[addr].pop(i)
	return data




def getScansPerDevicePerNode(data):
	scans = data["scans"]
	scansPerDev = {}
	minRssi = 127
	maxRssi = -127
	for nodeAddr in scans:
		for scan in scans[nodeAddr]:
			devAddr = scan["address"]
			timestamp = scan["time"]
			rssi = scan["rssi"]
			entry = {"time":timestamp, "rssi":rssi}
			if (devAddr not in scansPerDev):
				scansPerDev[devAddr] = {}

			if (nodeAddr not in scansPerDev[devAddr]):
				scansPerDev[devAddr][nodeAddr] = [entry]
			else:
				scansPerDev[devAddr][nodeAddr].append(entry)
			if (rssi > maxRssi):
				maxRssi = rssi
			if (rssi < minRssi):
				minRssi = rssi
	return {"scansPerDev":scansPerDev, "minRssi":minRssi, "maxRssi":maxRssi}


def getFrequencyPerDevicePerNode(data, windowSize):
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	data2 = getScansPerDevicePerNode(data)
	scansPerDev = data2["scansPerDev"]
	startTimes = range(int(startTimestamp), int(endTimestamp)+1, windowSize)
	numScans = {}
	for devAddr in scansPerDev:
		if (devAddr not in numScans):
			numScans[devAddr] = {}
		for tInd in range(1,len(startTimes)):
			for nodeAddr in scansPerDev[devAddr]:
				if (nodeAddr not in numScans[devAddr]):
					numScans[devAddr][nodeAddr] = [0.0]
				else:
					numScans[devAddr][nodeAddr].append(0.0)
				for scan in scansPerDev[devAddr][nodeAddr]:
					timestamp = scan["time"]
					rssi = scan["rssi"]
					if (startTimes[tInd-1] <= timestamp < startTimes[tInd]):
						numScans[devAddr][nodeAddr][-1] += 1.0
				# Beacon 0, 5, 9 scan six times faster atm
				if (nodeAddr in ["E8:00:93:4E:7B:D9", "F5:A7:4B:49:8C:7D", "FE:04:85:F9:8F:E9"]):
					numScans[devAddr][nodeAddr][-1] /= 6.0
	return {"numScansPerDev":numScans, "startTimes": startTimes}




def plotScansAsDots(data):
	scans = data["scans"]
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	i=0
	for nodeAddress in scans:
		print len(scans[nodeAddress])
		plt.figure(i)
		timestamps = {}
		for scan in scans[nodeAddress]:
			dev = scan["address"]
			if (dev not in timestamps):
				timestamps[dev] = [scan["time"] - startTimestamp]
			else:
				timestamps[dev].append(scan["time"] - startTimestamp)
		j=0
		for device in timestamps:
			plt.plot(timestamps[device], [j]*len(timestamps[device]), ".")
			j+=1
		names = []
		for device in timestamps:
			if (device in beaconNames):
				names.append(beaconNames[device])
			else:
				names.append(device)
		plt.yticks(range(0,j), names)

		nodeName = nodeAddress
		if (nodeAddress in beaconNames):
			nodeName = beaconNames[nodeAddress]
		plt.title("Devices scanned by " + nodeName)

		duration = endTimestamp-startTimestamp
		xticks = range(0, int(duration+1), int(duration/100))
		formattedTimestamps = []

		for timestamp in xticks:
			formattedTimestamps.append(datetime.datetime.fromtimestamp(timestamp+startTimestamp).strftime("%m-%d %H:%M"))
		plt.xticks(xticks, formattedTimestamps, rotation="vertical")

		plt.grid(axis="x")
#		# See http://stackoverflow.com/questions/12322738/how-do-i-change-the-axis-tick-font-in-a-matplotlib-plot-when-rendering-using-lat#12323891
#		fontDict = {"family":"sans-serif",
#					"sans-serif":["Helvetica"],
#					"weight" : "normal",
#					"size" : 12
#					}
#		fontDict = {
#			"family":"monospace",
#			"size" : 12
#		}
#		gca = plt.gca()
#		gca.set_xticklabels(formattedTimestamps, fontDict)
#		gca.set_yticklabels(names, fontDict)

		plt.axis([0, endTimestamp-startTimestamp, -0.5, j-0.5])
		i+=1


def plotScansAsDots2(data):
	scans = data["scans"]
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	plt.figure()
	i=0
	names = []
	for nodeAddr in scans:
#		print len(scans[nodeAddr])
#		plt.figure(i)
		timestamps = []
		for scan in scans[nodeAddr]:
			timestamps.append(scan["time"] - startTimestamp)
		plt.plot(timestamps, [i]*len(timestamps), ".")
#		plt.plot(timestamps, [i]*len(timestamps), ",")

		nodeName = nodeAddr
		if (nodeAddr in beaconNames):
			nodeName = beaconNames[nodeAddr]
		names.append(nodeName)
		i+=1

	plt.yticks(range(0,i), names)
	plt.title("Scan nodes")
	plt.axis([0, endTimestamp-startTimestamp, -0.5, i-0.5])



def plotRssi(data):
	scans = data["scans"]
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	data2 = getScansPerDevicePerNode(data)
	scansPerDev = data2["scansPerDev"]
	minRssi = data2["minRssi"]
	maxRssi = data2["maxRssi"]

	lineColors = ["b", "g", "r", "c", "m", "y", "k"]
	lineStyles = ["o", "^", "d", "+", "*"]
	for devAddr in scansPerDev:
#		plt.figure()
		fig, axarr = plt.subplots(len(scansPerDev[devAddr]), sharex=True, sharey=True)
		devName = devAddr
		if (devName in beaconNames):
			devName = beaconNames[devAddr]
#		plt.title("Scanned device: " + devName)
		i=0
		for nodeAddr in scansPerDev[devAddr]:
			timestamps = []
			rssis = []
			for scan in scansPerDev[devAddr][nodeAddr]:
				timestamp = scan["time"]
				rssi = scan["rssi"]
				timestamps.append(timestamp)
				rssis.append(rssi)
#			fmt = lineColors[i % len(lineColors)] + lineStyles[int(i/len(lineColors)) % len(lineStyles)]
			fmt = "b."
			nodeName = nodeAddr
			if (nodeAddr in beaconNames):
				nodeName = beaconNames[nodeAddr]
			subplot = axarr[i]
			if (len(scansPerDev[devAddr]) < 2):
				subplot = axarr
			subplot.plot(timestamps, rssis, fmt, alpha=0.3, label=nodeName)
			subplot.legend()
#			subplot.set_xlim([startTimestamp, endTimestamp])
			subplot.axis([startTimestamp, endTimestamp, minRssi, maxRssi])
			i+=1
#		plt.legend(loc="upper left")

		duration = endTimestamp-startTimestamp
		xticks = range(int(startTimestamp), int(endTimestamp+1), int(duration/100))
		formattedTimestamps = []
		for xtick in xticks:
			formattedTimestamps.append(datetime.datetime.fromtimestamp(xtick).strftime("%m-%d %H:%M"))

		if (len(scansPerDev[devAddr]) > 1):
			fig.subplots_adjust(hspace=0)
			axarr[0].set_title("Scanned device: " + devName)
			axarr[-1].set_xticks(xticks)
			axarr[-1].set_xticklabels(formattedTimestamps, rotation="vertical")
			plt.setp([a.get_xticklabels() for a in fig.axes[:-1]], visible=False)
		else:
			axarr.set_title("Scanned device: " + devName)
			axarr.set_xticks(xticks)
			axarr.set_xticklabels(formattedTimestamps, rotation="vertical")


def plotScanFrequency(data):
	scans = data["scans"]
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	windowSize = 10*60
	data2 = getFrequencyPerDevicePerNode(data, windowSize)
	numScans = data2["numScansPerDev"]
	startTimes = data2["startTimes"]

	lineColors = ["b", "g", "r", "c", "m", "y", "k"]
	lineStyles = ["o", "^", "d", "+", "*"]

	for devAddr in numScans:
#		plt.figure()
		fig, axarr = plt.subplots(len(numScans[devAddr]), sharex=True, sharey=True)
		devName = devAddr
		if (devName in beaconNames):
			devName = beaconNames[devAddr]

		i=0
		for nodeAddr in numScans[devAddr]:
#			fmt = lineColors[i % len(lineColors)] + lineStyles[int(i/len(lineColors)) % len(lineStyles)]
			fmt = "b-"
			nodeName = nodeAddr
			if (nodeAddr in beaconNames):
				nodeName = beaconNames[nodeAddr]
#			plt.plot(startTimes[1:], numScans[nodeAddr], fmt, alpha=0.3, label=nodeName)
			subplot = axarr[i]
			if (len(numScans[devAddr]) < 2):
				subplot = axarr
			subplot.plot(startTimes[1:], numScans[devAddr][nodeAddr], fmt, alpha=1.0, label=nodeName)
			subplot.legend()
			subplot.set_xlim([startTimestamp, endTimestamp])
			i+=1

		duration = endTimestamp-startTimestamp
		xticks = range(int(startTimestamp), int(endTimestamp+1), int(duration/100))
		formattedTimestamps = []
		for xtick in xticks:
			formattedTimestamps.append(datetime.datetime.fromtimestamp(xtick).strftime("%m-%d %H:%M"))
		if (len(numScans[devAddr]) > 1):
			fig.subplots_adjust(hspace=0)
			axarr[0].set_title("Scanned device: " + devName)
			axarr[-1].set_xticks(xticks)
			axarr[-1].set_xticklabels(formattedTimestamps, rotation="vertical")
			plt.setp([a.get_xticklabels() for a in fig.axes[:-1]], visible=False)
		else:
			axarr.set_title("Scanned device: " + devName)
			axarr.set_xticks(xticks)
			axarr.set_xticklabels(formattedTimestamps, rotation="vertical")



def plotBandwidth(data):
	# TODO: last value of numScans is wrong because that timeslot can be less than dt

	dt = 60

	scans = data["scans"]
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	startTimes = range(int(startTimestamp), int(endTimestamp)+dt, dt)
#	print startTimes
	numScans = {}
	numScans["total"] = [0]*len(startTimes)

	lineColors = ["b", "g", "r", "c", "m", "y", "k"]
	lineStyles = ["-o", "-^", "-d", "-+", "-*"]

	plt.figure()
	i = 0
	for node in scans:
		numScans[node] = [0]*len(startTimes)
		timeInd = 0
		for scan in scans[node]:
			scanTime = int(scan["time"])
			if (scanTime > startTimes[timeInd+1]):
				timeInd += 1
#			numScans[timeInd] += 1
			numScans[node][timeInd] += 1.0 / dt
			numScans["total"][timeInd] += 1.0 / dt
			scannerName = node
			if (node in beaconNames):
				scannerName = beaconNames[node]
		plt.title("Number of scanned devices per second")
		fmt = lineColors[i % len(lineColors)] + lineStyles[int(i/len(lineColors)) % len(lineStyles)]
		plt.plot(startTimes[0:-1], numScans[node][0:-1], fmt, label=scannerName)
		i+=1
	plt.legend(loc="upper left")

	plt.figure()
	plt.title("Total number of scanned devices per second")
	plt.plot(startTimes[0:-1], numScans["total"][0:-1])

def plotBandwidth2(data):
	# Time step
	dt = 60

	# Averaging window: must be divisible by 2*dt
	window = 300

	scans = data["scans"]
	startTimestamp = data["startTimestamp"]
	endTimestamp = data["endTimestamp"]
	startTimes = range(int(startTimestamp), int(endTimestamp)+dt, dt)
#	print startTimes
	numScans = {}
	numScans["total"] = [0]*len(startTimes)

	plt.figure()
	for node in scans:
		numScans[node] = [0]*len(startTimes)
		timeInd = 0
		for scan in scans[node]:
			scanTime = int(scan["time"])

			if (scanTime > startTimes[timeInd+1]):
				timeInd += 1

			tIndStart = max(timeInd - window/2/dt, 0)
			tIndEnd = min(timeInd + window/2/dt, len(startTimes)-1)
			for tInd in range(tIndStart, tIndEnd):
				numScans[node][tInd] += 1.0
				numScans["total"][tInd] += 1.0

		for timeInd in range(0, len(startTimes)):
			tIndStart = max(timeInd - window/2/dt, 0)
			tIndEnd = min(timeInd + window/2/dt, len(startTimes)-1)
			actualWindowSize = startTimes[tIndEnd] - startTimes[tIndStart]
			numScans[node][timeInd] /= actualWindowSize

		plt.title("Number of scanned devices per second (moving average)")
		plt.plot(startTimes[0:-1], numScans[node][0:-1])

	for timeInd in range(0, len(startTimes)):
		tIndStart = max(timeInd - window/2/dt, 0)
		tIndEnd = min(timeInd + window/2/dt, len(startTimes)-1)
		actualWindowSize = startTimes[tIndEnd] - startTimes[tIndStart]
		numScans["total"][timeInd] /= actualWindowSize

	plt.figure()
	plt.title("Total number of scanned devices per second (moving average)")
	plt.plot(startTimes[0:-1], numScans["total"][0:-1])


if __name__ == '__main__':
	print "File not intended as main."




