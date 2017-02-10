#!/usr/bin/python

import json
import os
import optparse
from copy import deepcopy
from collections import OrderedDict
from datetime import datetime
from shutil import copyfile
from termcolor import colored

###########################################
## LOG HELPER

def info(text):
	print colored(text, "yellow")

def log(text):
	print text

def warn(text):
	print colored(text, "magenta")

def err(text):
	print colored(text, "red")

###########################################

template = '{"stable": "","latest": "","versions": [],"time": {"modified": ""}}'

def getFile():
	path = os.environ['BLUENET_RELEASE_DIR']
	if path == "":
		err("BLUENET_RELEASE_DIR not defined")

	return "%s/index.json" %path

def loadIndex():
	return json.load(open(getFile(), 'r'), object_pairs_hook=OrderedDict)

def writeIndex(index):
	json.dump(index, open(getFile(), 'w'), indent=4, separators=(',', ': '))
	# json.dump(index, open(os.environ['BLUENET_RELEASE_DIR'] + '/index_out.json', 'w'), indent=4, separators=(',', ': '))

def addType(index, type):
	index[type.lower()] = json.loads(template, object_pairs_hook=OrderedDict)

def updateTime(index, type, version):
	timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
	index[type.lower()]['time']['modified'] = timestamp
	index[type.lower()]['time'][version] = timestamp

def addVersion(index, type, version, stable = True):
	if not type.lower() in index:
		addType(index, type)

	if hasVersion(index, type, version):
		err("Version already added, do you want to update timestamps? [y/N]")
		update=raw_input()
		if (update == 'y' or update == 'Y'):
			updateTime(index, type, version)
			return True
		else:
			return False
	else:
		index[type.lower()]['latest'] = version
		index[type.lower()]['versions'].append(version)

		if stable:
			index[type.lower()]['stable'] = version

		updateTime(index, type, version)
		return True

def hasVersion(index, type, version):
	return version in index[type.lower()]['versions']



if __name__ == '__main__':

	try:
		parser = optparse.OptionParser(usage='%prog -t <type> -v <version>')

		parser.add_option('-t', '--type',
			action='store',
			dest='type',
			type='string',
			default=None,
			help='Type: Crownstone, Guidestone, Bootloader'
		)
		parser.add_option('-v', '--version',
			action='store',
			dest='version',
			type='string',
			default=None,
			help='Version to be added'
		)
		parser.add_option('-s', '--stable',
			action='store_true',
			dest='stable',
			help='Set version as stable'
		)

		options, args = parser.parse_args()


		if not options.type or not options.version:
			parser.print_help()
			exit(1)

		log("Loading existing index ...")
		index = loadIndex()

		log("Updating ...")
		if not addVersion(index, options.type, options.version, options.stable):
			info("Nothing to update. Done")
			exit(0)

		log("Creating backup of old index ...")
		copyfile(getFile(), getFile() + '.bak')

		log("Writing new index ...")
		writeIndex(index)

		info("Done")

	except Exception, e:
		err("Exception " + e)
		exit(1)
