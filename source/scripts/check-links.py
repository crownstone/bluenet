#!/usr/bin/python3

import os
import re

DOCS_DIR_PREFIX = "../"
DOCS_DIR = "../docs/"
FILENAMES = [DOCS_DIR_PREFIX + DOCS_DIR + F for F in ["PROTOCOL.md", "BEHAVIOUR.md", "SERVICE_DATA.md", "SERVICE_DATA_DEPRECATED.md", "BROADCAST_PROTOCOL.md", "UART_PROTOCOL.md", "MESH_PROTOCOL.md", "IPC.md"]]

patternHeaderLink = re.compile(".*\]\(#([^\)]+)\)") # Matches: [some text](#header-to-link-to)
patternHeaderLinkExternal = re.compile(".*\]\(([^#]+)#([^\)]+)\)") # Matches: [some text](File.md#header-to-link-to)
patternWebLink = re.compile("http.*")
patternHeader = re.compile("^#+ (.+)")

headers = {}

def getHeaders(filename):
	if filename in headers:
		return headers[filename]

#	print(f"getHeaders {filename}")
	headers[filename] = []
	try:
		with open(filename, 'r') as file:
			lines = file.readlines()
			for line in lines:
				match = patternHeader.match(line)
				if match:
					# Github replaces spaces by dashes, and makes it all lower case.
					header = match.group(1).replace(' ', '-').lower()
					headers[filename].append(header)
	except:
		pass

	return headers[filename]

def checkLinks(filename):
	file = open(filename)
	lines = file.readlines()

	for i in range(0, len(lines)):
		line = lines[i]
		lineNr = i + 1
		match = patternHeaderLink.match(line)
		if match:
			link = match.group(1)
			if link not in getHeaders(filename):
				print(f"{filename}:{lineNr} Link not found: {link}\n    {line}")
			else:
#				print(f"found {link}")
				pass

		match = patternHeaderLinkExternal.match(line)
		if match:
			externalFilename = match.group(1)
			link = match.group(2)

			if not patternWebLink.match(externalFilename):
				externalFilename = os.path.dirname(filename) + '/' + externalFilename
				if link not in getHeaders(externalFilename):
					print(f"{filename}:{lineNr} Link not found: {link}\n    {line}")
				else:
#					print(f"found {link}")
					pass



for filename in FILENAMES:
	print(f"Checking links of {filename} ..")
	checkLinks(filename)