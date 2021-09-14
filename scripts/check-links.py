#!/usr/bin/python3

import os
import re
import numpy as np

DOCS_DIR = "../docs/"
FILENAMES = [DOCS_DIR + F for F in [
"PROTOCOL.md",
"BEHAVIOUR.md",
"SERVICE_DATA.md",
"SERVICE_DATA_DEPRECATED.md",
"BROADCAST_PROTOCOL.md",
"UART_PROTOCOL.md",
"MESH_PROTOCOL.md",
"IPC.md",
"ASSET_FILTER_STORE.md"]]

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
			linkResult(filename, line, lineNr, filename, link)

		match = patternHeaderLinkExternal.match(line)
		if match:
			externalFilename = match.group(1)
			link = match.group(2)

			if not patternWebLink.match(externalFilename):
				externalFilename = os.path.dirname(filename) + '/' + externalFilename
				linkResult(filename, line, lineNr, externalFilename, link)


def linkResult(filename, line, lineNr, linkFilename, link):
	if link not in getHeaders(linkFilename):
		print(f"{filename}:{lineNr} Link not found: \"{link}\"\n    {line.rstrip()}")
		suggestion = getSuggestion(linkFilename, link)
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

def levenshtein(string1, string2):
	# See https://en.wikipedia.org/wiki/Levenshtein_distance
	size1 = len(string1) + 1
	size2 = len(string2) + 1
	dist = np.zeros((size1, size2))
	for i in range(0, size1):
		dist[i, 0] = i
	for j in range(0, size2):
		dist[0, j] = j

	for i in range(1, size1):
		for j in range(1, size2):
			substitutionCost = 1
			if string1[i - 1] == string2[j - 1]:
				substitutionCost = 0

			dist[i, j] = min(dist[i - 1, j] + 1,                      # deletion
			                 dist[i, j - 1] + 1,                      # insertion
			                 dist[i - 1, j - 1] + substitutionCost    # substitution
			                 )

	return (dist[size1 - 1, size2 - 1])


for filename in FILENAMES:
	print(f"Checking links of {filename} ..")
	checkLinks(filename)
