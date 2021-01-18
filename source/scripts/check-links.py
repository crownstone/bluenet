#!/usr/bin/python3

import os
import re
import numpy as np

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
			linkResult(filename, line, lineNr, link)

		match = patternHeaderLinkExternal.match(line)
		if match:
			externalFilename = match.group(1)
			link = match.group(2)

			if not patternWebLink.match(externalFilename):
				externalFilename = os.path.dirname(filename) + '/' + externalFilename
				linkResult(externalFilename, line, lineNr, link)


def linkResult(filename, line, lineNr, link):
	if link not in getHeaders(filename):
		print(f"{filename}:{lineNr} Link not found: \"{link}\"\n    {line.rstrip()}")
		suggestion = getSuggestion(filename, link)
		if suggestion:
			print(f"Did you mean \"{suggestion}\"?")
		print()
	else:
#		print(f"found {link}")
		pass


def getSuggestion(filename, link):
	minDistance = 100000000000
	bestMatch = ""
	for header in headers[filename]:
		distance = levenshtein(header, link)
		if distance < minDistance:
			minDistance = distance
			bestMatch = header
	return bestMatch

def levenshtein(seq1, seq2):
	size_x = len(seq1) + 1
	size_y = len(seq2) + 1
	matrix = np.zeros((size_x, size_y))
	for x in range(size_x):
		matrix[x, 0] = x
	for y in range(size_y):
		matrix[0, y] = y

	for x in range(1, size_x):
		for y in range(1, size_y):
			if seq1[x - 1] == seq2[y - 1]:
				matrix[x, y] = min(
					matrix[x - 1, y] + 1,
					matrix[x - 1, y - 1],
					matrix[x, y - 1] + 1
				)
			else:
				matrix[x, y] = min(
					matrix[x - 1, y] + 1,
					matrix[x - 1, y - 1] + 1,
					matrix[x, y - 1] + 1
				)
	return (matrix[size_x - 1, size_y - 1])


for filename in FILENAMES:
	print(f"Checking links of {filename} ..")
	checkLinks(filename)