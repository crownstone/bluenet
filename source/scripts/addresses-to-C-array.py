#!/usr/bin/python

import sys
import re

pattern = re.compile("([0-9A-F]{2}).([0-9A-F]{2}).([0-9A-F]{2}).([0-9A-F]{2}).([0-9A-F]{2}).([0-9A-F]{2})")

addressesFile = open(sys.argv[1], "r")
numAddresses = 0
for line in addressesFile:
#	print line
	matches = pattern.findall(line)
#	print matches
	if (len(matches) > 0):
		addressString = ""
		for i in reversed(range(0,6)):
			addressString += "0x" + matches[0][i] + ", "
		print addressString
		numAddresses += 1

print "Number of addresses:",  numAddresses
addressesFile.close()