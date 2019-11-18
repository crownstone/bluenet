#!/usr/bin/env python3

import re
import os

STATE_TYPE_FILE =   "../include/common/cs_Types.h"
STATE_ACCESS_FILE = "../src/common/cs_Types.cpp"
PROTOCOL_FILE = "../../docs/PROTOCOL.md"
OUTPUT_FILE = "access.md"

ACCESS_LEVELS = ["ADMIN", "MEMBER", "BASIC", "NO_ONE", ]
ACCESS_LEVELS_DICT = {
    "ADMIN": 1,
    "MEMBER": 2,
    "BASIC": 3,
    "NO_ONE": 255
}
ACCESS_LEVEL_NO_ONE = "NO_ONE"

STATE_TYPE_START_PATTERN = re.compile("^enum class CS_TYPE")
STATE_TYPE_PATTERN =re.compile("\s*(\S+)\s+=\s*(\d+)")

STATE_ACCESS_GET_FUN_PATTERN = re.compile(".*\sgetUserAccessLevelGet\(CS_TYPE const & type\)")
STATE_ACCESS_SET_FUN_PATTERN = re.compile(".*\sgetUserAccessLevelSet\(CS_TYPE const & type\)")
CASE_PATTERN = re.compile(".*\scase CS_TYPE::(.+?):")

accessLevels = "|".join(ACCESS_LEVELS)
STATE_RETURN_ACCESS_LEVEL_PATTERN = re.compile(".*\sreturn\s+(" + accessLevels + ")")

PROTOCOL_STATE_TYPES_START_PATTERN = re.compile("<a name=\"state_types\"></a>")
PROTOCOL_STATE_TYPES_TABLE_START_PATTERN = re.compile("Type nr \| Type name \| Payload type \| Description")
PROTOCOL_STATE_TYPES_TABLE_END_PATTERN = re.compile('<a name=')

# Pattern for: "8 | iBeacon UUID | uint 8 [16] | iBeacon UUID."
PROTOCOL_STATE_TYPES_TABLE_ENTRY_PATTERN = re.compile("(\d+)(\s+\|\s+[^|]+){3}")

def parseStateTypes():
    file = open(STATE_TYPE_FILE, "r")
    bracketLevel = 0
    foundStart = False
    stateTypes = {}
    for line in file:
        if (foundStart and bracketLevel == 0):
            #print("end of enum:", line)
            foundStart = False
            break
        match = STATE_TYPE_START_PATTERN.match(line)
        if (match):
            foundStart = True
        if (foundStart):
            bracketLevel += line.count('{')
            bracketLevel -= line.count('}')
            match = STATE_TYPE_PATTERN.match(line)
            if (match):
                stateName = match.group(1)
                stateNum = match.group(2)
                stateTypes[stateName] = stateNum
    file.close()
    return stateTypes


def parseAccess(fileName, typeDict, functionPattern, casePattern, accessReturnPattern):
    file = open(fileName, "r")
    bracketLevel = 0
    foundFun = False
    access = {}
    cases = []
    for line in file:
#        print(line)
        if (foundFun and bracketLevel == 0):
            #print("end of function:", line)
            foundFun = False
            break

        match = functionPattern.match(line)
        if (match):
            #print("found function:", line)
            foundFun = True
        if (foundFun):
            bracketLevel += line.count('{')
            bracketLevel -= line.count('}')

            match = casePattern.match(line)
            if (match):
                typeName = match.group(1)
                # print("found case of type", typeName, " Line:", line)
                cases.append(typeName)

            match = accessReturnPattern.match(line)
            if (match):
                accessLevelName = match.group(1)
                # print("found access level", accessLevelName, " Line:", line)
                for typeName in cases:
                    typeNum = typeDict.get(typeName, None)
                    if (typeNum != None):
                        access[typeNum] = accessLevelName
                cases.clear()
    file.close()
    return access

def parseProtocol(startPattern, tableStartPattern, tableEndPattern, entryPattern):
    file = open(PROTOCOL_FILE, "r")
    foundStart = False
    foundTable = False
    tableHeader = ""
    tableEntries = {}
    for line in file:
        match = startPattern.match(line)
        if (match):
            # print("Found start:", line)
            foundStart = True
        if (foundStart):
            match = tableStartPattern.match(line)
            if (match):
                # print("Found table:", line)
                foundTable = True
                tableHeader = match.group(0)
        if (foundTable):
            match = entryPattern.match(line)
            if (match):
                typeNum = match.group(1)
                entry = match.group(0).rstrip()
                # print("Found entry", typeNum, ":", entry)
                tableEntries[typeNum] = entry
            match = tableEndPattern.match(line)
            if (match):
                foundStart = False
                foundTable = False
    file.close()
    return (tableHeader, tableEntries)


def genTable(tableHeader, tableEntries, types, accessGet, accessSet):
    file = open(OUTPUT_FILE, "w")
    header = tableHeader
    numHeaders = header.count('|') + 1
    numAccessLevels = 0
    for levelName in ACCESS_LEVELS:
        if levelName == ACCESS_LEVEL_NO_ONE:
            continue
        header += " | " + levelName[0]
        numAccessLevels += 1
    file.write(header + "\n")
    header2 = ""
    headerCounter = 0
    for headerField in header.split('|'):
        if (headerCounter < numHeaders):
            header2 += '-' * max(3, len(headerField) - 1)
            header2 += " | "
        else:
            header2 += ":---: | "
    header2 = header2[0:-3]
    file.write(header2 + "\n")

    for typeNum, entry in tableEntries.items():
        for levelName in ACCESS_LEVELS:
            if levelName == ACCESS_LEVEL_NO_ONE:
                continue
            readAllowed = False
            levelNum = ACCESS_LEVELS_DICT[levelName]
            minLevelName = accessGet[typeNum]
            minLevelNum = ACCESS_LEVELS_DICT[minLevelName]
            if (isAllowed(levelNum, minLevelNum)):
               readAllowed = True

            writeAllowed = False
            minLevelName = accessSet[typeNum]
            minLevelNum = ACCESS_LEVELS_DICT[minLevelName]
            if (isAllowed(levelNum, minLevelNum)):
                writeAllowed = True
            # if (readAllowed or writeAllowed):
            entry += " | "
            if (readAllowed):
                entry += "r"
            if (writeAllowed):
                entry += "w"
        file.write(entry + "\n")
    file.close()


def isAllowed(level, minLevel):
    # print("isAllowed level:", level, "minLevel:", minLevel)
    if minLevel == ACCESS_LEVELS_DICT[ACCESS_LEVEL_NO_ONE]:
        # print("noone")
        return False
    if level <= minLevel:
        # print("allow")
        return True
    return False

# Get a dict with type name -> type num
stateTypes = parseStateTypes()

stateGetAccess = parseAccess(STATE_ACCESS_FILE, stateTypes, STATE_ACCESS_GET_FUN_PATTERN, CASE_PATTERN, STATE_RETURN_ACCESS_LEVEL_PATTERN)
stateSetAccess = parseAccess(STATE_ACCESS_FILE, stateTypes, STATE_ACCESS_SET_FUN_PATTERN, CASE_PATTERN, STATE_RETURN_ACCESS_LEVEL_PATTERN)

# print(stateGetAccess)
# print(stateSetAccess)
# print(stateTypes)
# for stateName, stateNum in stateTypes.items():
#     print(stateNum, stateName, stateGetAccess[stateNum], stateSetAccess[stateNum])

(stateTableHeader, stateTableEntries) = parseProtocol(
    PROTOCOL_STATE_TYPES_START_PATTERN,
    PROTOCOL_STATE_TYPES_TABLE_START_PATTERN,
    PROTOCOL_STATE_TYPES_TABLE_END_PATTERN,
    PROTOCOL_STATE_TYPES_TABLE_ENTRY_PATTERN
)
genTable(stateTableHeader, stateTableEntries, stateTypes, stateGetAccess, stateSetAccess)


