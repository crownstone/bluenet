#!/bin/sh

verbose=$1

if [ $verbose ]; then
	echo "This can also be obtained through sd_ble_version_get()"
	echo "It is written at memory address 0x0000300C except for very old softdevice firmwares."
fi
softdevice_firmware_version=$(./readbytes.sh 000300C 2)

if [ $verbose ]; then
	echo "We read softdevice_firmware_version: $softdevice_firmware_version"
fi

# Table can be found at: https://devzone.nordicsemi.com/f/nordic-q-a/1171/how-do-i-access-softdevice-version-string

case $softdevice_firmware_version in
	0079)
		version="S132 v2.0.0-7.alpha"
		echo $version
		;;
	0081)
		version="S132 v2.0.0"
		echo $version
		;;
	0088)
		version="S132 v2.0.1"
		echo $version
		;;
	008C)
		version="S132 v3.0.0"
		echo $version
		;;
	0091)
		version="S132 v3.1.0"
		echo $version
		;;
	0098)
		version="S132 v4.0.2"
		echo $version
		;;
	009D)
		version="S132 v5.0.0"
		echo $version
		;;
	00A0)
		version="S132 v5.0.1"
		echo $version
		;;
	00A5)
		version="S132 v5.1.0"
		echo $version
		;;
	00A8)
		version="S132 v6.0.0"
		echo $version
		;;
	FFFE)
		version="development/any"
		echo $version
		;;
	*)
		echo "Sorry, I don't know this version"
		;;
esac
