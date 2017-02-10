#!/usr/bin/env python

#TODO: crc calculation has a problem for bootloader and softdevice,
# but works for application ??!!

import sys
import os
import optparse
import time
from intelhex import IntelHex
# import manifestTemplates
import shutil
import struct
import json
import copy
from os.path import basename
from _utils import Logger, LogLevel

manifestTemplate = {
	"manifest": {
		"dfu_version": 0.8
	}
}

deviceTemplate = {
	"bin_file": "",
	"dat_file": "",
	"init_packet_data": {
		"application_version": 4294967295,
		"device_revision": 65535,
		"device_type": 65535,
		"firmware_crc16": 0,
		"softdevice_req": []
	}
}

def hexFileToArr(filename):
	log.i("filename" + filename)
	ih = IntelHex(filename)
	buf = ih.tobinarray()
	log.d("End and start of the binary: " + str(ih._get_start_end()))
	return buf

def crc16_ccitt(arr8, length):
	"""
	:param arr8:
	:param length:
	:return:
	"""
	crc = 0xFFFF
	for i in range(0, length):
		crc = (crc >> 8 & 0xFF) | (crc << 8 & 0xFFFF)
		crc ^= arr8[i]
		crc ^= (crc & 0xFF) >> 4
		crc ^= (crc << 8 & 0xFFFF) << 4 & 0xFFFF
		crc ^= ((crc & 0xFF) << 4 & 0xFFFF) << 1 & 0xFFFF
	return crc

def crcFromHex(filename):
	buf = hexFileToArr(filename)
	len_buf = len(buf)
	barename = os.path.splitext(basename(filename))[0]
	log.d("Total length: " + str(len_buf) + " (check with size " + barename + ".bin on the commandline and resulting zip file)")
	crc = crc16_ccitt(buf, len_buf)
	crc_hex_st = hex(crc)
	crc_int_st = str(int(crc_hex_st, 16))
	log.d("Calculated crc16_ccitt: " + crc_hex_st + "(hex) or " + crc_int_st + " (dec)")
	return crc

def verifyFile(filename):
	if not os.path.isfile(filename):
		# log.e("Hex file not found!")
		print "Hex file not found!"
		exit(2)
	path, ext = os.path.splitext(filename)
	if ext != '.hex':
		# log.e("Only HEX file upload supported")
		print "Only HEX file upload supported"
		exit(2)

def createDatFile(name, crc, sd_req):
	log.i("Create .dat file")
	buf = struct.pack('<IHHHHH', 0xffffffff, 0xffff, 0xffff, 1, int(sd_req, 16), crc)
	file = open('dfu_tmp/%s.dat' %name, 'wb')
	file.write(buf)
	file.close()

def createBinFile(filename, name):
	#        bname = basename(name);
	buf = hexFileToArr(filename)
	file = open('dfu_tmp/%s.bin' %name, 'wb')
	file.write(buf)
	file.close()

def createManifest(type, name, crc, sd_req, manifest = None):
	if not manifest:
		log.i("Create manifest")
		manifest = copy.deepcopy(manifestTemplate)
	else:
		log.i("Append manifest")
	device = copy.deepcopy(deviceTemplate)
	device["bin_file"] = '%s.bin' %name
	device["dat_file"] = '%s.dat' %name
	device["init_packet_data"]["firmware_crc16"] = crc
	device["init_packet_data"]["softdevice_req"].append(int(sd_req, 16))
	manifest["manifest"][type] = device
	return manifest

def writeManifest(manifest):
	file = open('dfu_tmp/manifest.json', 'w')
	json.dump(manifest, file, indent = 4, sort_keys = True)
	file.close()

def createDFU(type, filename, sd_req, manifest = None):
	verifyFile(filename)

	path, nameExt = os.path.split(filename)
	name, ext = os.path.splitext(nameExt)

	# os.chdir('tmp')
	crc = crcFromHex(filename)
	createDatFile(name, crc, sd_req)
	createBinFile(filename, name)

	return createManifest(type, name, crc, sd_req, manifest)

def prepare():
	log.i("Create tmp directory")
	os.mkdir('dfu_tmp')

def writeToZip(path, type):
	log.i("Create " + type + ".zip file")
	if not path:
		filename = type
	else:
		filename = path + '/' + type

	shutil.make_archive(filename, 'zip', base_dir='.', root_dir='dfu_tmp')
	# shutil.make_archive(type, 'zip', 'tmp')


def createApplicationDFU(filename, sd_req):
	log.i("Create application dfu file")
	prepare()
	manifest = createDFU("application", filename, sd_req)
	writeManifest(manifest);
	writeToZip(os.path.dirname(filename), "application");

def createBootloaderDFU(filename, sd_req):
	prepare()
	manifest = createDFU("bootloader", filename, sd_req)
	writeManifest(manifest);
	writeToZip(os.path.dirname(filename), "bootloader");

def createSoftdeviceDFU(filename, sd_req):
	prepare()
	manifest = createDFU("softdevice", filename, sd_req)
	writeManifest(manifest);
	writeToZip(os.path.dirname(filename), "softdevice");

def createCombinedBlAppDFU(bootloaderFilename, applicationFilename, sd_req):
	prepare()
	manifest = createDFU("bootloader", bootloaderFilename, sd_req)
	manifest = createDFU("application", applicationFilename, sd_req, manifest)
	writeManifest(manifest);
	writeToZip(os.path.dirname(bootloaderFilename), "combined_bl_app");

if __name__ == '__main__':

	print os.getcwd()

	log = Logger(LogLevel.DEBUG)
	try:
		parser = optparse.OptionParser(usage='%prog -a <application_hex> -b <bootloader_hex> -s <softdevice_hex> -r <required_softdevice>\n\nExample:\n\t%prog.py -a crownstone.hex',
									   version='0.1')

		parser.add_option('-a', '--application',
				action='store',
				dest="application_hex",
				type="string",
				default=None,
				help='Application hex file'
		)
		parser.add_option('-b', '--bootloader',
				action='store',
				dest="bootloader_hex",
				type="string",
				default=None,
				help='Bootloader hex file'
		)
		parser.add_option('-s', '--softdevice',
				action='store',
				dest="softdevice_hex",
				type="string",
				default=None,
				help='Softdevice hex file'
		)
		parser.add_option('-c', '--combined',
				action='store_true',
				dest="combined",
				help='Create combined zip'
		)
		parser.add_option('-r', '--sd-req',
				action='store',
				dest="sd_req",
				type="string",
				default="0xfffe",
				help='supported Soft Device'
		)
		parser.add_option('-v', '--verbose',
				action='store_true',
				dest="verbose",
				help='Be verbose.'
		)
		parser.add_option('-d', '--debug',
				action='store_true',
				dest="debug",
				help='Debug mode.'
		)

		options, args = parser.parse_args()

	except Exception, e:
		log.e(str(e))
		log.i("For help use --help")
		exit(2)

	# if (options.application_hex and options.bootloader_hex) or \
	#    (options.application_hex and options.softdevice_hex) or \
	#    (options.bootloader_hex and options.softdevice_hex):
	# 	log.e("Only single update supported for now!")
	# 	exit(2)

	if not (options.application_hex or options.bootloader_hex or options.softdevice_hex):
		log.e("At least one hex file has to be provided!")
		parser.print_help()
		exit(2)

	if (options.verbose):
		log.setLevel(bleLog.D)
	if (options.debug):
		log.setLevel(bleLog.D2)

	try:
		if options.combined:
			if options.application_hex and options.bootloader_hex:
				createCombinedBlAppDFU(options.bootloader_hex, options.application_hex, options.sd_req)
		else:
			if options.application_hex:
				createApplicationDFU(options.application_hex, options.sd_req)

			if options.bootloader_hex:
				createBootloaderDFU(options.bootloader_hex, options.sd_req)

			if options.softdevice_hex:
				createSoftdeviceDFU(options.softdevice_hex, options.sd_req)
	finally:
		log.i("Remove tmp directory, recursively")
		shutil.rmtree('dfu_tmp')
