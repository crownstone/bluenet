#!/usr/bin/env python2

from termcolor import colored
import time, datetime

###########################################
## LOG HELPER

def info(text):
	print colored(text, "blue")

def log(text):
	print text
	print text

def warn(text):
	print colored(text, "yellow")

def err(text):
	print colored(text, "red")

###########################################

class LogLevel:
	ERROR =   0
	WARNING = 1
	INFO =    2
	DEBUG =   3
	DEBUG2 =  4

class LogType:
	ERROR =   0
	WARNING = 1
	INFO =    2
	DEBUG =   3
	SUCCESS = 10

class Logger:
	def __init__(self, logLevel=LogLevel.ERROR, timestamps=True):
		self.logLevel = logLevel
		self.timestamps = timestamps

	def log(self, logLevel, logType, string):
		if (logLevel > self.logLevel):
			return
		printStr = ""
		printStr += self._typeToColor(logType)
		if (self.timestamps):
			#printStr += str(datetime.datetime.now()) + " "
			# TODO: let the user define a format
			printStr += datetime.datetime.fromtimestamp(time.time()).strftime("[%Y-%m-%d %H:%M:%S] ")
		printStr += string

		# End color
		printStr += "\033[0m"
		print printStr

	def e(self, string, logType=LogType.ERROR):
		self.log(LogLevel.ERROR, logType, string)

	def w(self, string, logType=LogType.WARNING):
		self.log(LogLevel.WARNING, logType, string)

	def i(self, string, logType=LogType.INFO):
		self.log(LogLevel.INFO, logType, string)

	def d(self, string, logType=LogType.DEBUG):
		self.log(LogLevel.DEBUG, logType, string)

	def dd(self, string, logType=LogType.DEBUG):
		self.log(LogLevel.DEBUG2, logType, string)

	def getLogLevel(self):
		return self.logLevel

	def setLogLevel(self, logLevel):
		self.logLevel = logLevel

	def _typeToColor(self, logType):
		return self._logColor.get(logType, "")

	_logColor = {
		LogType.ERROR   : '\033[91m',  # red
		LogType.WARNING : '\033[93m',  # yellow
		LogType.INFO    : '\033[94m',  # blue
		LogType.DEBUG   : '\033[95m',  # purple
		LogType.SUCCESS : '\033[92m',  # green
	}
