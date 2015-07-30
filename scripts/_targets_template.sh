#!/bin/bash
#
# template for the _targets.sh file. Copy this file over to $BLUENET_CONFIG_DIR and
# rename to _targets.sh. It will update the target name and the serial numbers used
# by the scripts:
#   - firmware.sh
#   - softdevice.sh
#   - all.sh (bootloader)
# and enable compiling for different targets without losing previously compiled files
# as well as enable you to flash different devices without having to unplug / replug them
# all the time
#
# How to use:
#
# 1. Define target names, e.g. sirius and capella
# 2. create sub folders in the $BLUENET_CONFIG_DIR with the name of the targets, e.g. $BLUENET_CONFIG_DIR/sirius
#    and $BLUENET_CONFIG_DIR/capella
# 3. copy and rename the $BLUENET_DIR/CMakeBuild.config.default to $BLUENET_CONFIG_DIR/<target name>/CMakeBuild.config
# 4. Find out the serial number of the device using JLinkExe, e.g.
#     $ JLinkExe
#       > ShowEmuList
#       J-Link[0]: Connection: USB, Serial number: 480207700, ProductName: J-Link-OB-SAM3U128
#       J-Link[1]: Connection: USB, Serial number: 480110849, ProductName: J-Link-OB-SAM3U128
# 3. copy and rename this file to $BLUENET_CONFIG_DIR/_targets.sh
# 4. add the targets and serial numbers further below where it says ENTER NEW TARGETS HERE
#
# Now you can call the scripts mentioned earlier together with the target name at the end, e.g.
#
# ./firmware run sirius
# ./softdevice.sh all capella
# ./all.sh sirius
#
# the compiled files will then be copied to the folder $BLUENET_CONFIG_DIR/<target name>/build/
#
# Note: You can still use the scripts without a target supplied, in which case it will use the CMakeBuild.config in
#       $BLUENET_CONFIG_DIR and compile to $BLUENET_CONFIG_DIR/build. But you won't be able to program them while
#       having more than one connected!

target=$1

if [[ $target != "crownstone" ]]; then
	BLUENET_ORIG_CONFIG_DIR=$BLUENET_CONFIG_DIR
	BLUENET_CONFIG_DIR=$BLUENET_CONFIG_DIR${target:+/$target}
	BLUENET_ORIG_BUILD_DIR=$BLUENET_BUILD_DIR
	BLUENET_BUILD_DIR=$BLUENET_BUILD_DIR${target:+/$target}
fi

case "$target" in
	# ENTER NEW TARGETS HERE in the form of:
	# <target name>)
	#   serial_num=<serial number>
	#   ;;
	#
	# for example:
	#
	# sirius)
	# 	serial_num=480110849
	#	;;
	# capella)
	#	serial_num=480207700
	#	;;
esac
target=crownstone
