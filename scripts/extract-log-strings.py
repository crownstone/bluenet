#!/usr/bin/env python3
import argparse
import json
import os
import re
import traceback
from enum import Enum

defaultSourceFilesDir = os.path.abspath(f"{os.path.dirname(os.path.abspath(__file__))}/../build/default/CMakeFiles/crownstone.dir/src")
defaultTopDir = "bluenet/source/"

argParser = argparse.ArgumentParser(description="Extract log strings from source files, to be used by the log client.")
argParser.add_argument('--sourceFilesDir',
                       '-s',
                       dest='sourceFilesDir',
                       metavar='path',
                       type=str,
                       default=f"{defaultSourceFilesDir}",
                       help='The path with the pre-compiled bluenet source code files on your system (.i or .ii files)')
argParser.add_argument('--topDir',
                       '-t',
                       dest='topDir',
                       metavar='path',
                       type=str,
                       default=f"{defaultTopDir}",
                       help="File names will include the path starting at the root dir, like: /home/joe/workspace/bluenet/source/src/cs_Crownstone.cpp. We want them to start at a dir that's in the repository, like: bluenet/source/src/cs_Crownstone.cpp. That dir is found by the first occurrence of this string.")
argParser.add_argument('--outputFile',
                       '-f',
                       dest='outputFileName',
                       metavar='path',
                       type=str,
                       default="extracted_logs.json",
                       help='The output file.')
argParser.add_argument('--verbose',
                       '-v',
                       dest="verbose",
                       action='store_true',
                       help='Show verbose output')
args = argParser.parse_args()

class LogType(Enum):
    NONE = 0
    LOG = 1
    ARRAY = 2

class LogStringExtractor:
    def __init__(self, debug=False):
        self.debugOuput = debug

        # if (6 <= 7) { cs_log_args(fileNameHash("/home/bluenet-workspace/bluenet/source/src/mesh/cs_MeshCore.cpp", sizeof("/home/bluenet-workspace/bluenet/source/src/mesh/cs_MeshCore.cpp")), 64, 6, true, "cs_mesh_write_cb handle=%u retCode=%u", handle, retCode); };
        self.logPattern = re.compile(".*?cs_log_args\((.*)")

        # if (7 <= 7) { cs_log_array(fileNameHash("/home/bluenet-workspace/bluenet/source/src/mesh/cs_MeshCore.cpp", sizeof("/home/bluenet-workspace/bluenet/source/src/mesh/cs_MeshCore.cpp")), 455, 7, true, false, nrf_mesh_configure_device_uuid_get(), (16), "{", "}", " - ", "0x%02X"); };
        self.logArrayPattern = re.compile(".*?cs_log_array\((.*)")

        self.sourceFilesDir = None

        # Key:   filename hash
        # Value: filename
        self.fileNames = {}

        # Key:   filename hash
        # Value: map with:
        #        Key:   line number
        #        Value: log string
        self.logs = {}

        # Key:   filename hash
        # Value: map with:
        #        Key:   line number
        #        Value: (startFormat, endFormat, separationFormat, elementFormat)
        self.logArrays = {}

    def parse(self, sourceFilesDir: str, outputFile: str, topDir: str):
        self.setSourceFilesDir(sourceFilesDir)
        self._parseFiles()
        self._exportToFile(outputFile, topDir)

    # We could also get all source files from: build/default/CMakeFiles/crownstone.dir/depend.internal
    def setSourceFilesDir(self, dir: str):
        if os.path.isdir(dir) == False:
            print(f"No such dir: {dir}")

        self.sourceFilesDir = dir

    def _parseFiles(self):
        """
        Cache all C/C++ file names in sourceFilesDir
        """
        for root, dirs, files in os.walk(self.sourceFilesDir):
            for fileName in files:
                if fileName.endswith((".cpp.ii", ".c.i", ".hpp.ii")):
                    self._parseFile(os.path.join(root, fileName))

    def _parseFile(self, fileName):
        file = open(fileName, "r")
        lines = file.readlines()
        file.close()

        mergedLine = ""

        mergingMultiLine = LogType.NONE
        bracketOpenCount = 0
        for line in lines:
            if line.startswith('#'):
                # Skip comments.
                continue

            if mergingMultiLine != LogType.NONE:
                # Continuation of a previously found log that did not end at the same line.
                mergedLine += line.strip()
                bracketOpenCount += self._countBrackets(line)

                if bracketOpenCount == 0:
                    # This is the last line of the multi line log.
                    # print(f"Merged multiline: {mergedLine}")
                    if mergingMultiLine == LogType.LOG:
                        self._parseLogLine(mergedLine)
                    elif mergingMultiLine == LogType.ARRAY:
                        self._parseLogArrayLine(mergedLine)
                    mergingMultiLine = LogType.NONE
                continue

            match = self.logPattern.match(line)
            if match:
                bracketOpenCount = self._countBrackets(line)
                if bracketOpenCount > 0:
                    # This is a multi line log, start merging multiple lines.
                    mergingMultiLine = LogType.LOG
                    mergedLine = line.strip()
                    continue
                if bracketOpenCount < 0:
                    print(f"Too many closing brackets:")
                    print(f"File: {fileName}")
                    print(f"Line: {line}")
                    return
                self._parseLogLine(line)

            match = self.logArrayPattern.match(line)
            if match:
                bracketOpenCount = self._countBrackets(line)
                if bracketOpenCount > 0:
                    # This is a multi line log, start merging multiple lines.
                    mergingMultiLine = LogType.ARRAY
                    mergedLine = line.strip()
                    continue
                if bracketOpenCount < 0:
                    print(f"Too many closing brackets:")
                    print(f"File: {fileName}")
                    print(f"Line: {line}")
                    return
                self._parseLogArrayLine(line)

    def _parseLogLine(self, line):
        # print(f"Found: {line}")
        match = self.logPattern.match(line)
        (endIndex, logArgs) = self._getArgs(match.group(1), 0)
        logCode = match.group(1)[0:endIndex]
        # print(match.group(1))
        # print(logCode)
        # print(logArgs)
        if logArgs is None or not logArgs[0].startswith("fileNameHash("):
            return
        (endIndex, fileNameHashArgs) = self._getArgs(logArgs[0], len("fileNameHash("))
        # print(fileNameHashArgs)

        fileName = fileNameHashArgs[0][1:-1] # Remove quotes from string
        fileNameHash = self._getFileNameHash(fileName)
        lineNumber = int(logArgs[1])
        # logLevel = int(logArgs[2])
        # addNewLine = logArgs[3]
        logString = self._removeQuotes(logArgs[4])
        # print(f"{fileNameHash} {lineNumber} {logString}")
        if fileNameHash not in self.logs:
            self.logs[fileNameHash] = {}
        self.logs[fileNameHash][lineNumber] = logString
        self.fileNames[fileNameHash] = fileName

    def _parseLogArrayLine(self, line):
        #  if (7 <= 7) { cs_log_array(
        #  0    fileNameHash(
        #          "/home/bluenet-workspace/bluenet/source/src/mesh/cs_MeshCore.cpp",
        #          sizeof("/home/bluenet-workspace/bluenet/source/src/mesh/cs_MeshCore.cpp")
        #      ),
        #  1   456,
        #  2   7,
        #  3   true,
        #  4   false,
        #  5   nrf_mesh_configure_device_uuid_get(),
        #  6   (16),
        #  7   "[",
        #  8   "]",
        #  9   " - ",
        #  10  "%02X, "
        #  ); };
        # print(f"Line: {line}")
        match = self.logArrayPattern.match(line)
        (endIndex, logArgs) = self._getArgs(match.group(1), 0)
        logCode = match.group(1)[0:endIndex]
        # print(f"match.group(1): {match.group(1)}")
        # print(f"logCode: {logCode}")
        # print(f"logArgs: {logArgs}")
        if logArgs is None or not logArgs[0].startswith("fileNameHash("):
            return
        (endIndex, fileNameHashArgs) = self._getArgs(logArgs[0], len("fileNameHash("))
        # print(fileNameHashArgs)

        try:
            fileName = fileNameHashArgs[0][1:-1] # Remove quotes from string
            fileNameHash = self._getFileNameHash(fileName)
            lineNumber = int(logArgs[1])
            # logLevel = int(logArgs[2])
            # addNewLine = logArgs[3]
            # reverse = logArgs[4]
            startFormat = self._removeQuotes(logArgs[7])
            endFormat = self._removeQuotes(logArgs[8])
            separationFormat = self._removeQuotes(logArgs[9])
            elementFormat = None
            if len(logArgs) > 10:
                elementFormat = self._removeQuotes(logArgs[10])

            if fileNameHash not in self.logArrays:
                self.logArrays[fileNameHash] = {}
            self.logArrays[fileNameHash][lineNumber] = (startFormat, endFormat, separationFormat, elementFormat)
            self.fileNames[fileNameHash] = fileName
        except Exception as e:
            print(f"Failed to parse line: {line}")
            if self.debugOuput:
                print(f"Extracted args: {logArgs}")
                traceback.print_exc()
        pass

    def _countBrackets(self, line):
        escape = False
        string = None # Can become either ' or "
        bracketOpenCount = 0
        for c in line:
            if escape:
                escape = False
                continue
            if c == '\\':
                escape = True
                continue

            if string != None:
                if c == string:
                    # End of string
                    string = None
                continue

            if c == '"' or c == "'":
                string = c
                # Start of string
                continue

            if c == '(':
                bracketOpenCount += 1
            if c == ')':
                bracketOpenCount -= 1
        return bracketOpenCount

    # Returns index of closing bracket, or None if not found
    # startIndex is index after the opening bracket
    def _getArgs(self, line, startIndex):
        escape = False
        string = None # Can become either ' or "
        bracketOpenCount = 1
        args = [""]
        for i in range(startIndex, len(line)):
            c = line[i]
            if escape:
                escape = False
                args[-1] += c
                continue
            if c == '\\':
                escape = True
                continue

            args[-1] += c
            if string != None:
                if c == string:
                    # End of string
                    string = None
                continue

            if c == '"' or c == "'":
                string = c
                # Start of string
                continue

            if c == '(':
                bracketOpenCount += 1
            if c == ')':
                bracketOpenCount -= 1
            if bracketOpenCount == 1 and c == ',':
                # Remove comma from arg
                args[-1] = args[-1][0:-1]
                args.append("")
            if bracketOpenCount == 0:
                # Remove the last closing bracket from the arg
                args[-1] = args[-1][0:-1]

                # Remove leading and trailing spaces from args
                for j in range(0, len(args)):
                    args[j] = args[j].strip()

                return i, args
        return None, None




    def getFileName(self, fileNameHash: int):
        try:
            return self.fileNames[fileNameHash]
        except:
            return None

    def getLogFormat(self, fileName: str, lineNumber: int):
        fileNameHash = self._getFileNameHash(fileName)
        try:
            return self.logs[fileNameHash][lineNumber]
        except:
            return None

    def getLogArrayFormat(self, fileNameHash: int, lineNumber: int):
        try:
            return self.logArrays[fileNameHash][lineNumber]
        except:
            return (None, None, None, None)


    def _getFileNameHash(self, fileName: str):
        byteArray = bytearray()
        byteArray.extend(map(ord, fileName))

        hashVal: int = 5381
        # A string in C ends with 0.
        hashVal = (hashVal * 33 + 0) & 0xFFFFFFFF
        for c in reversed(byteArray):
            if c == ord('/'):
                return hashVal
            hashVal = (hashVal * 33 + c) & 0xFFFFFFFF
        return hashVal

    def _removeQuotes(self, line: str):
        """
        Removes quotes that make a string, and concatenates strings.
        Example: '"This is just an " "example"'
        Will return: 'This is just an example'
        """
        escape = False
        inQuotes = False
        result = ""
        for c in line:
            if escape:
                escape = False
                result += c
                continue
            if c == '\\':
                escape = True
                continue
            if c == '"':
                inQuotes = not inQuotes
                continue
            if inQuotes:
                result += c
        return result

    def _exportToFile(self, outputFileName: str, topDir: str):
        self.fileCleanupPattern = re.compile(f".*?({topDir}.*)")

        output = {
            "source_files": [],
            "logs": [],
            "logs_array": []
        }

        for fileNameHash, fileName in self.fileNames.items():
            match = self.fileCleanupPattern.match(fileName)
            if not match:
                print(f"Failed to cleanup file {fileName}")
                return
            fileName = match.group(1)
            output["source_files"].append({
                "file_hash": fileNameHash,
                "file_name": fileName
            })

        for fileNameHash, val in self.logs.items():
            for lineNr, fmt in val.items():
                output["logs"].append({
                    "file_hash": fileNameHash,
                    "line_nr": lineNr,
                    "log_fmt": fmt
                })

        for fileNameHash, val in self.logArrays.items():
            for lineNr, fmt in val.items():
                output["logs_array"].append({
                    "file_hash": fileNameHash,
                    "line_nr": lineNr,
                    "start_fmt": fmt[0],
                    "end_fmt": fmt[1],
                    "separator_fmt": fmt[2],
                    "element_fmt": fmt[3]
                })

        with open(outputFileName, 'w') as jsonFile:
            json.dump(output, jsonFile)

parser = LogStringExtractor(debug=args.verbose)
parser.parse(args.sourceFilesDir, args.outputFileName, args.topDir)

