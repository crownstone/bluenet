#!/usr/bin/python2

import re
import pygame
from pygame.locals import *
from random import randint
import os

DOCS_DIR_PREFIX = "../"
DOCS_DIR = "../docs/"
DIR = "diagrams/"
GEN_DIR = DOCS_DIR + DIR
FILENAMES = [DOCS_DIR + F for F in ["PROTOCOL.md", "BEHAVIOUR.md", "SERVICE_DATA.md", "SERVICE_DATA_DEPRECATED.md", "BROADCAST_PROTOCOL.md", "UART_PROTOCOL.md", "MESH_PROTOCOL.md", "IPC.md", "MESH.md", "BLUETOOTH.md"]]

fontPath = DOCS_DIR + "diagrams/fonts/LiberationSans-Regular.ttf"
fontSizeBlocks = 24
fontSizeBytes = 16

STEP_X = 50
STEP_Y = 150
MARGIN = 12
BOX_WIDTH = 4
MAX_VAR_LEN = 6
DEFAULT_VAR_LEN = 6

# Define the colors we will use in RGB format
BLACK = (  0,   0,   0,   0)
WHITE = (255, 255, 255,   0)
BLUE =  (  0,   0, 255,   0)
GREEN = (  0, 255,   0,   0)
RED =   (255,   0,   0,   0)

TRANSPARENT = (255, 255, 255, 128)

GREYS = [None]*16
GREYS[4]  = (178, 178, 178,   0)

SKY_BLUES = [None]*16
SKY_BLUES[2]  = (51, 153, 255,   0)
SKY_BLUES[3]  = (0, 102, 255,   0)
SKY_BLUES[6]  = (0, 0, 128,   0)
SKY_BLUES[7]  = (0, 51, 102,   0) # Dark
SKY_BLUES[8]  = (51, 102, 153,   0)
SKY_BLUES[9]  = (102, 153, 204,   0)
SKY_BLUES[10] = (153, 204, 255,   0) # Light

GREENS = [None]*16
GREENS[4]  = (0, 153, 0,   0)
GREENS[5]  = (0, 102, 0,   0)
GREENS[7]  = (51, 102, 51,   0)
GREENS[8]  = (102, 153, 102,   0)
GREENS[9]  = (153, 204, 153,   0)
GREENS[10] = (204, 255, 204,   0)

REDS = [None]*16
REDS[3]    = (255, 0, 0,   0)
REDS[5]    = (153, 0, 0,   0)
REDS[7]    = (102, 51, 51,   0)
REDS[8]    = (153, 102, 102,   0)
REDS[9]    = (204, 153, 153,   0)
REDS[10]   = (255, 204, 204,   0)

YELLOWS = [None]*16
YELLOW_7  = (102, 102, 51,   0)
YELLOW_8  = (153, 153, 102,   0)
YELLOW_9  = (204, 204, 153,   0)
YELLOW_10 = (255, 255, 204,   0)
YELLOWS = [YELLOW_7, YELLOW_8, YELLOW_9, YELLOW_10]

PURPLES = [None]*16
PURPLES[4]  = (102, 0, 204,   0)
PURPLES[5]  = (51, 0, 153,   0)
PURPLES[7]  = (51, 0, 102,   0)
PURPLES[8]  = (102, 51, 153,   0)
PURPLES[9]  = (153, 102, 204,   0)
PURPLES[10] = (204, 153, 255,   0)

CHARTS = [None]*16
CHARTS[1]  = (  0,  69, 134,   0) # Blue
CHARTS[2]  = (255,  66,  14,   0) # Orange
CHARTS[3]  = (255, 255, 255,   0) # Yellow
CHARTS[4]  = ( 87, 157,  28,   0) # Green
CHARTS[5]  = (126,   0,  33,   0) # Brown
CHARTS[6]  = (131, 202, 255,   0) # Light blue
CHARTS[7]  = ( 49,  64,   4,   0) # Dark green
CHARTS[8]  = (174, 207,   0,   0) # Light green
CHARTS[9]  = ( 75,  31, 111,   0) # Purple
CHARTS[10] = (255, 149,  14,   0) # Orange
CHARTS[11] = (197,   0,  11,   0) # Red
CHARTS[12] = (  0, 132, 209,   0) # Blue




COLOR_PALETTE = []
# COLOR_PALETTE.extend(SKY_BLUES[7:10])
# COLOR_PALETTE.extend(GREENS[7:10])
# COLOR_PALETTE.extend(REDS[7:10])
# COLOR_PALETTE.extend(PURPLES[7:10])
# COLOR_PALETTE.extend(CHARTS[1:13])
COLOR_PALETTE.extend([SKY_BLUES[6], SKY_BLUES[3], SKY_BLUES[2]])
COLOR_PALETTE.extend([REDS[3], REDS[5]])
COLOR_PALETTE.extend([GREENS[4], GREENS[5]])
COLOR_PALETTE.extend([PURPLES[4], PURPLES[5]])
COLOR_PALETTE.extend([CHARTS[2], CHARTS[10]])
# COLOR_PALETTE.extend([GREYS[4]])


# All text in lower case
colorDict = {}


colorDict['type'] = SKY_BLUES[3]
colorDict['data type'] = SKY_BLUES[3]
colorDict['device type'] = SKY_BLUES[2]
colorDict['length'] = SKY_BLUES[2]
colorDict['size'] = SKY_BLUES[2]
colorDict['ad length'] = SKY_BLUES[6]
colorDict['ad type'] = SKY_BLUES[3]
colorDict['flags'] = SKY_BLUES[2]

colorDict['etc'] = GREYS[4]
colorDict['reserved'] = GREYS[4]
colorDict['padding'] = GREYS[4]
colorDict['rand'] = GREYS[4]

# Payload, data, etc
colorDict['payload'] = CHARTS[2]
colorDict['encrypted payload'] = CHARTS[2]
colorDict['data'] = CHARTS[2]
colorDict['data part'] = CHARTS[2]
colorDict['list'] = CHARTS[2]
colorDict['encrypted data'] = CHARTS[2]
colorDict['service data'] = CHARTS[2]
colorDict['command payload'] = CHARTS[2]
colorDict['command data'] = CHARTS[2]

os.environ['SDL_VIDEODRIVER'] = 'dummy'

def drawRect(rect, color):
	# Rect: left, top, width, height
	# width of 0 to fill
        # print(rect)
	s = pygame.Surface((rect[2], rect[3]))  # the size of your rect
	#s.set_alpha(128)                # alpha level
	s.fill((color))           # this fills the entire surface
	background.blit(s, (rect[0],rect[1]))    # (0,0) are the top-left coordinates


def drawTextLines(x, y, labels, width, height, vertical, zoom):
	maxLineWidth = 0
	maxLineHeight = 0
	for label in labels:
		if (label.get_width() > maxLineWidth):
			maxLineWidth = label.get_width()
		if (label.get_height() > maxLineHeight):
			maxLineHeight = label.get_height()
	maxLineWidth = maxLineWidth * zoom
	maxLineHeight = maxLineHeight * zoom
	textWidth = maxLineWidth
	textHeight = maxLineHeight*len(labels)

	xCenter = x + 0.5*width
	yCenter = y + 0.5*height

	if vertical:
		for i in range(0, len(labels)):
			rotated = pygame.transform.rotozoom(labels[i], -90, zoom)
			drawY = yCenter - 0.5*labels[i].get_width()*zoom
			drawX = xCenter - 0.5*textHeight + (len(labels) - 1 - i) * maxLineHeight
			background.blit(rotated, (drawX, drawY))

	else:
		for i in range(0, len(labels)):
			zoomed = pygame.transform.rotozoom(labels[i], 0, zoom)
			drawX = xCenter - 0.5*labels[i].get_width()*zoom
			drawY = yCenter - 0.5*textHeight + i*maxLineHeight
			background.blit(zoomed, (drawX, drawY))
			


# Return zoom required to fit
def calcTextZoom(labels, width, height, vertical):
	maxLineWidth = 0
	maxLineHeight = 0
	for label in labels:
		if (label.get_width() > maxLineWidth):
			maxLineWidth = label.get_width()
		if (label.get_height() > maxLineHeight):
			maxLineHeight = label.get_height()
	textWidth = maxLineWidth
	textHeight = maxLineHeight*len(labels)

	if vertical:
		zoom = 1.0*height / textWidth
		horZoom = 1.0*width / textHeight
		if (horZoom < zoom):
			zoom = horZoom
	else:
		zoom = 1.0*width / textWidth
		horZoom = 1.0*height / textHeight
		if (horZoom < zoom):
			zoom = horZoom
	if zoom > 1.0:
		return 1.0
	return zoom


def drawText(x, y, text, color, width, height, forceVertical=False):
	# IDEA: Try to find a fit with the highest zoom.
	# Options are horizontal/vertical and replacing spaces with line breaks

	if (forceVertical):
		verticals = [True]
	else:
		verticals = [False, True]

	# Keep up best required zoom
	bestZoom = 0.0
	bestLines = []
	bestVert = False

	# First try without line breaks
	lines = [text]
	labels = []
	for line in lines:
		# The text can only be a single line: newline characters are not rendered.
		label = fontBlocks.render(line, True, WHITE)
		labels.append(label)
	for vert in verticals:
		zoom = calcTextZoom(labels, width, height, vert)
		if (zoom > bestZoom):
			bestZoom = zoom
			bestLines = lines
			bestLabels = labels
			bestVert = vert

	# Try 1 line break
	if (' ' in text):
		splitText = text.split(' ')
		for i in range(1,len(splitText)):
			# Build up the string with 1 line break
			lines = ["", ""]
			for line in splitText[0:i]:
				lines[0] = lines[0] + " " + line
			for line in splitText[i:]:
				lines[1] = lines[1] + " " + line
			labels = []
			for line in lines:
				# The text can only be a single line: newline characters are not rendered.
				label = fontBlocks.render(line, True, WHITE)
				labels.append(label)

			# Calculate the required zoom for this text
			for vert in verticals:
				zoom = calcTextZoom(labels, width, height, vert)
				if (zoom > bestZoom):
					bestZoom = zoom
					bestLines = lines
					bestLabels = labels
					bestVert = vert

	# Draw text with best zoom
	drawTextLines(x, y, bestLabels, width, height, bestVert, bestZoom)

	

def drawVar(startX, y, varName, varLen, color):
	if varLen > MAX_VAR_LEN:
		varLen = MAX_VAR_LEN
	width = varLen*STEP_X
	height = STEP_Y
	
	drawRect([startX, 0, width-2, y-2], color)
	drawRect([startX, y, width-2, height], color)
	drawText(startX+MARGIN, y+MARGIN, varName, WHITE, width-2*MARGIN, height-2*MARGIN, varLen < 3)
	return startX + width


def drawVarList(varList, filename, lengthInBits):
	if not filename:
		print("no filename for:")
		for var in varList:
			print("  " + var[0])
		return
	print("Generating " + filename)

	totalLen = 0
	for var in varList:
		# print var[0] + " " + str(var[1])
		if (var[1] > MAX_VAR_LEN):
			totalLen += MAX_VAR_LEN
		else:
			totalLen += var[1]

	size = [totalLen * STEP_X, STEP_Y]

	# Add text "byte" or "bit" to screen size
	byteTxt = "bit" if lengthInBits else "byte"
	byteLabel = fontBytes.render(byteTxt, True, WHITE)
	size[0] += byteLabel.get_width()
	size[1] += byteLabel.get_height()

	global screen
	screen = pygame.display.set_mode(size, pygame.DOUBLEBUF, 32)
	screen.convert_alpha()

	#screen.fill(TRANSPARENT)

	global background
	background = pygame.Surface(size, pygame.SRCALPHA, 32)
	background.set_alpha(128)

	#screen.blit(background, (0,0))
	x=0
	y=0

	# Draw the text "byte"
	xVar = x + byteLabel.get_width()
	yVar = y + byteLabel.get_height()
	
        drawRect([0, 0, xVar-2, yVar], BLACK)
	background.blit(byteLabel, (x, y))
	
        x += byteLabel.get_width() + 2

	cycleColorInd = 0
	prevColorInd = -1
	byteNum = 0
	byteNumKnown = True
	for var in varList:
		varName = var[0]
		varLen = var[1]
		varLenKnown = var[2]

		# Determine color
		# First check if this var name already has an assigned color
		varNameLower = varName.lower()
		# print "in dict " + varNameLower + "=" + str(varNameLower in colorDict)
		if varNameLower in colorDict:
			color = colorDict[varNameLower]
		else:
			# Don't use the same color as the previous color
			colorInd = prevColorInd
			while colorInd == prevColorInd:
				# colorInd = randint(0, len(COLOR_PALETTE)-1)
				colorInd = cycleColorInd
				cycleColorInd = (cycleColorInd + 1) % len(COLOR_PALETTE)
			color = COLOR_PALETTE[colorInd]
			colorDict[varNameLower] = color

		# Keep up last used color index
		if (color in COLOR_PALETTE):
			prevColorInd = COLOR_PALETTE.index(color)
		else:
			prevColorInd = -1

		xVar = drawVar(xVar, yVar, varName, varLen, color)

		# Draw the byte numbers
		if byteNumKnown:
			if varLenKnown:
				if varLen > MAX_VAR_LEN:
					endByteNum = byteNum + varLen-1
					for i in range(0, MAX_VAR_LEN-2):
						byteLabel = fontBytes.render(str(byteNum), True, WHITE)
						background.blit(byteLabel, (x + 0.5*STEP_X - 0.5*byteLabel.get_width(), y))
						byteNum += 1
						x += STEP_X
					byteLabel = fontBytes.render("...", True, WHITE)
					background.blit(byteLabel, (x + 0.5*STEP_X - 0.5*byteLabel.get_width(), y))
					byteNum += 1
					x += STEP_X
					byteNum = endByteNum
					byteLabel = fontBytes.render(str(byteNum), True, WHITE)
					background.blit(byteLabel, (x + 0.5*STEP_X - 0.5*byteLabel.get_width(), y))
					byteNum += 1
					x += STEP_X

				else:
					for i in range(0, varLen):
						byteLabel = fontBytes.render(str(byteNum), True, WHITE)
						background.blit(byteLabel, (x + 0.5*STEP_X - 0.5*byteLabel.get_width(), y))
						byteNum += 1
						x += STEP_X
			else:
				byteLabel = fontBytes.render(str(byteNum), True, WHITE)
				background.blit(byteLabel, (x + 0.5*STEP_X - 0.5*byteLabel.get_width(), y))
				byteLabel = fontBytes.render("...", True, WHITE)
				background.blit(byteLabel, (x + 1.5*STEP_X - 0.5*byteLabel.get_width(), y))
				byteNumKnown = False


	pygame.image.save(background, filename)


def parseFile(textFilename):
	file = open(textFilename)

	filename = None
	foundTableHeader=False
	lengthInBits=False
	foundTableLines=False
	varList = []

	for line in file:
		#print "line: " line
		if (foundTableLines):
			match = patternTableRow.findall(line)
			if (len(match)):
				#print "found line: " + line
				varName = match[0][0].strip()
				linkMatch = patternLink.findall(varName)
				if (linkMatch):
					varName = linkMatch[0]

				varLenKnown = True
				try:
					varLen = int(match[0][1].strip())
				except:
					varLenKnown = False
					varLen = DEFAULT_VAR_LEN

				varList.append((varName, varLen, varLenKnown))
				#print "varName=" + varName + " varLen=" + str(varLen)

			else:
				# End of table, draw and reset
				drawVarList(varList, filename, lengthInBits)
				filename = None
				foundTableHeader=False
				foundTableLines = False
				varList = []


		# Just skip one line
		if (foundTableHeader):
			#print "foundTableLines"
			foundTableLines = True
			foundTableHeader = False

		matches = patternTableHeader.findall(line)
		if len(matches):
			#print "foundTableHeader: " + matches[0]
			lengthInBits = (matches[0] == "in bits")
			foundTableHeader = True

		matches = patternFileName.findall(line)
		if len(matches):
			filename = GEN_DIR + matches[0]

	# End of file
	if (foundTableLines):
		# Draw last table
		drawVarList(varList, filename, lengthInBits)

if not os.path.exists(GEN_DIR):
	print("Make dir " + GEN_DIR)
	os.makedirs(GEN_DIR)

pygame.init()
pygame.display.init()

#myFont = pygame.font.SysFont("monospace", 15)
fontBlocks = pygame.font.Font(fontPath, fontSizeBlocks)
fontBytes = pygame.font.Font(fontPath, fontSizeBytes)

# Regex patterns
patternFileNameString = "\\(" + DOCS_DIR + DIR + "([^\\)]+)\\)"
patternFileName = re.compile(patternFileNameString)
patternTableHeader = re.compile("Type +\\| +Name +\\| +Length (in bits)? *\\| +Description")
patternTableRow = re.compile("[^|]\\|([^|]+)\\|([^|]+)\\|.*")
patternLink = re.compile("\\[([^]]+)\\]\\([^\\)]+\\)")

for filename in FILENAMES:
	print(filename)
	parseFile(filename)
	

# pygame.display.flip()

# done = False
# while not done:
# 	for event in pygame.event.get(): # User did something
# 		if event.type == pygame.QUIT: # If user clicked close
# 			done=True # Flag that we are done so we exit this loop

pygame.quit()

