#!/bin/bash

#############################################################################
# Create a new release
#
# This will:
#  - Create a new CMakeBuild.config in $BLUENET_DIR/release/model_version/
#  - Build firmware, bootloader, DFU packages and docs and copy
#    them to $BLUENET_RELEASE_DIR/model_version/
#  - Update the index.json file in $BLUENET_RELEASE_DIR to keep
#    track of stable, latest, and release dates (for cloud)
#  - Create a Change Log file from git commits since last release
#  - Create a git tag with the version number
#
#############################################################################

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source $BLUENET_DIR/scripts/_utils.sh

############################
### Pre Script Verification
############################

if [ -z $BLUENET_RELEASE_DIR ]; then
	cs_err "BLUENET_RELEASE_DIR is not defined!"
	exit 1
fi
if [ -z $BLUENET_BOOTLOADER_DIR ]; then
	cs_err "BLUENET_BOOTLOADER_DIR is not defined!"
	exit 1
fi

pushd $BLUENET_DIR &> /dev/null

# check current branch, releases should be made from master branch
branch=$(git symbolic-ref --short -q HEAD)

if [[ $branch != "master" ]]; then
	cs_err "You are currently on branch '"$branch"'"
	cs_err "Releases should be made from master branch"
	cs_err "Are you sure you want to continue? [y/N]"
	read branch_response
	if [[ ! $branch_response == "y" ]]; then
		cs_info "abort"
		exit 1
	fi
fi

# check for modifications in bluenet code
modifications=$(git ls-files -m | wc -l)

if [[ $modifications != 0 ]]; then
	cs_err "There are modified files in your bluenet code"
	cs_err "Commit the files or stash them first!!"
	exit 1
fi

# check for untracked files
untracked=$(git ls-files --others --exclude-standard | wc -l)

if [[ $untracked != 0 ]]; then
	cs_err "The following untracked files were found in the blunet code"
	cs_err "Make sure you didn't forget to add anything important!"
	git ls-files --others --exclude-standard
	cs_info "Do you want to continue? [Y/n]"
	read untracked_response
	if [[ $untracked_response == "n" ]]; then
		exit 1
	fi
fi

# check for remote updates
git remote update

# if true then there are remote updates that need to be pulled first
if [ ! $(git rev-parse HEAD) = $(git ls-remote $(git rev-parse --abbrev-ref @{u} | sed 's/\// /g') | cut -f1) ]; then
	cs_err "There are remote updates that were not yet pulled"
	cs_err "Are you sure you want to continue? [y/N]"
	read update_response
	if [[ ! $update_response == "y" ]]; then
		cs_info "abort"
		exit 1
	fi
fi

popd &> /dev/null

############################
### Prepare
############################

# check version number
# enter version number?

valid=0
existing=0

# Get old version number
if [ -f $BLUENET_DIR/VERSION ]; then
	version_str=`cat $BLUENET_DIR/VERSION`
	version_list=(`echo $version_str | tr '.' ' '`)
	v_major=${version_list[0]}
	v_minor=${version_list[1]}
	v_patch=${version_list[2]}
	cs_log "Current version: $version_str"
	v_minor=$((v_minor + 1))
	v_patch=0
	suggested_version="$v_major.$v_minor.$v_patch"
else
	suggested_version="1.0.0"
fi

# Ask for new version number
while [[ $valid == 0 ]]; do
	cs_info "Enter a version number [$suggested_version]:"
	read -e version
	if [[ $version == "" ]]; then
		version=$suggested_version
	fi

	if [[ $version =~ ^[0-9]{1,2}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then

		cs_info "Select model:"
		printf "$yellow"
		options=("Crownstone" "Guidestone")
		select opt in "${options[@]}"
		do
		    case $opt in
		        "Crownstone")
					model="crownstone"
					# device_type="DEVICE_CROWNSTONE_PLUG"
					break
		            ;;
		        "Guidestone")
					model="guidestone"
					# device_type="DEVICE_GUIDESTONE"
					break
		            ;;
		        *) echo invalid option;;
		    esac
		done
		printf "$normal"

		directory=$BLUENET_DIR/release/$model"_"$version

		if [ -d $directory ]; then
			cs_err "Version already exists, are you sure? [y/N]: "
			read version_response
			if [[ $version_response == "y" ]]; then
				existing=1
				valid=1
			fi
		else
			valid=1
		fi
	else
		cs_err "Version does not match pattern"
	fi
done

#####################################
### Create Config file and directory
#####################################

# create directory in bluenet with new config file. copy from default and open to edit

if [[ $existing == 0 ]]; then
	cs_log "Creating new directory: "$directory
	mkdir $directory &> /dev/null

	cp $BLUENET_DIR/conf/cmake/CMakeBuild.config.default $directory/CMakeBuild.config

###############################
### Fill Default Config Values
###############################

	if [[ $model == "crownstone" ]]; then
		sed -i "s/BLUETOOTH_NAME=\".*\"/BLUETOOTH_NAME=\"Crown\"/" $directory/CMakeBuild.config
	elif [[ $model == "guidestone " ]]; then
		sed -i "s/BLUETOOTH_NAME=\".*\"/BLUETOOTH_NAME=\"Guide\"/" $directory/CMakeBuild.config
	fi

	sed -i "s/FIRMWARE_VERSION=\".*\"/FIRMWARE_VERSION=\"$version\"/" $directory/CMakeBuild.config
	# sed -i "s/DEVICE_TYPE=.*/DEVICE_TYPE=$device_type/" $directory/CMakeBuild.config

	sed -i "s/NRF51822_DIR=/#NRF51822_DIR=/" $directory/CMakeBuild.config
	sed -i "s/COMPILER_PATH=/#COMPILER_PATH=/" $directory/CMakeBuild.config

	sed -i "s/CROWNSTONE_SERVICE=.*/CROWNSTONE_SERVICE=1/" $directory/CMakeBuild.config
	sed -i "s/INDOOR_SERVICE=.*/INDOOR_SERVICE=0/" $directory/CMakeBuild.config
	sed -i "s/GENERAL_SERVICE=.*/GENERAL_SERVICE=0/" $directory/CMakeBuild.config
	sed -i "s/POWER_SERVICE=.*/POWER_SERVICE=0/" $directory/CMakeBuild.config
	sed -i "s/SCHEDULE_SERVICE=.*/SCHEDULE_SERVICE=0/" $directory/CMakeBuild.config

	sed -i "s/PERSISTENT_FLAGS_DISABLED=.*/PERSISTENT_FLAGS_DISABLED=0/" $directory/CMakeBuild.config
	sed -i "s/BLUETOOTH_NAME=\".*\"/BLUETOOTH_NAME=\"Crown\"/" $directory/CMakeBuild.config
	sed -i "s/SERIAL_VERBOSITY=.*/SERIAL_VERBOSITY=SERIAL_NONE/" $directory/CMakeBuild.config
	sed -i "s/DEFAULT_OPERATION_MODE=.*/DEFAULT_OPERATION_MODE=OPERATION_MODE_SETUP/" $directory/CMakeBuild.config

	xdg-open $directory/CMakeBuild.config &> /dev/null

	if [[ $? != 0 ]]; then
		cs_info "Open $directory/CMakeBuild.config in to edit the config"
	fi

	cs_log "After editing the config file, press [ENTER] to continue"
	read
else
	cs_warn "Warn: Using existing configuration"
fi

cs_info "Stable version? [Y/n]: "
read stable
if [[ $stable == "n" ]]; then
	stable=0
else
	stable=1
fi

########################
### Update release index
########################

# NOTE: do this before modifying the paths otherwise BLUENET_RELEASE_DIR will point the the subdirectory
#   but the index file is located in the root directory

# goto bluenet scripts dir
pushd $BLUENET_DIR/scripts &> /dev/null

cs_info "Update release index ..."
if [[ $stable == 1 ]]; then
	./update_release_index.py -t $model -v $version -s
else
	./update_release_index.py -t $model -v $version
fi

checkError "Failed"
cs_succ "Copy DONE"

popd &> /dev/null

############################
###  modify paths
############################

export BLUENET_CONFIG_DIR=$directory
export BLUENET_BUILD_DIR=$BLUENET_BUILD_DIR/$model"_"$version
export BLUENET_RELEASE_DIR=$BLUENET_RELEASE_DIR/$model"_"$version
export BLUENET_BIN_DIR=$BLUENET_RELEASE_DIR/bin

############################
### Run
############################

###################
### Config File
###################

#create new config directory in release directory
mkdir -p $BLUENET_RELEASE_DIR/config

cs_info "Copy configuration to release dir ..."
cp $BLUENET_CONFIG_DIR/CMakeBuild.config $BLUENET_RELEASE_DIR/config

checkError
cs_succ "Copy DONE"

###################
### Softdevice
###################

# goto bluenet scripts dir
pushd $BLUENET_DIR/scripts &> /dev/null

cs_info "Build softdevice ..."
./softdevice.sh build

checkError
cs_succ "Softdevice DONE"

###################
### Firmware
###################

cs_info "Build firmware ..."
./firmware.sh -c release

checkError
cs_succ "Build DONE"
popd &> /dev/null

###################
### Bootloader
###################

# goto bootloader scripts dir
pushd $BLUENET_BOOTLOADER_DIR/scripts &> /dev/null

cs_info "Build bootloader ..."
./all.sh

checkError
cs_succ "Build DONE"

popd &> /dev/null

###################
### DFU
###################

# goto bluenet scripts dir
pushd $BLUENET_DIR/scripts &> /dev/null

cs_info "Create DFU packages ..."
./dfuGenPkg.py -a "$BLUENET_BIN_DIR/crownstone.hex" -o $model"_"$version

bootloaderVersion=$(cat $BLUENET_BOOTLOADER_DIR/include/version.h | \
	grep BOOTLOADER_VERSION | awk -F "\"" '{print $2}')

./dfuGenPkg.py -b "$BLUENET_BIN_DIR/bootloader_dfu.hex" -o "bootloader_"$bootloaderVersion

checkError
cs_succ "DFU DONE"

popd &> /dev/null

# build doc and copy to release directory

###################
### Documentation
###################

# goto bluenet scripts dir
pushd $BLUENET_DIR/scripts &> /dev/null

cs_info "Build docs ..."
./documentation.sh generate

checkError
cs_succ "Build DONE"

cs_info "Copy docs to relase dir ..."
cp -r $BLUENET_DIR/docs $BLUENET_RELEASE_DIR/docs

checkError
cs_succ "Copy DONE"

cs_info "Publish docs to git ..."
#git add $BLUENET_DIR/docs
#git commit -m "Update docs"
./documentation.sh publish

checkError
cs_succ "Published docs DONE"

popd &> /dev/null

####################
### GIT release tag
####################

pushd $BLUENET_DIR &> /dev/null

cs_info "Add release config"

# add new generated config to git
git add $directory
git commit -m "Add release config for "$model"_"$version

cs_info "Create git tag for release"

# if old version existed
if [[ $version_str ]]; then
	echo $version > VERSION

	cs_log "Updating changes overview"
	echo "Version $version:" > tmpfile
	git log --pretty=format:" - %s" "v$version_str"...HEAD >> tmpfile
	echo "" >> tmpfile
	echo "" >> tmpfile
	cat CHANGES >> tmpfile
	mv tmpfile CHANGES

	cs_log "Add to git"
	git add CHANGES VERSION
	git commit -m "Version bump to $version"

	cs_log "Create tag"
	git tag -a -m "Tagging version $version" "v$version"

	# log "Push tag"
	# git push origin --tags
else
	# setup first time
	echo $version > VERSION

	cs_log "Creating changes overview"
	git log --pretty=format:" - %s" >> CHANGES
	echo "" >> CHANGES
	echo "" >> CHANGES

	cs_log "Add to git"
	git add VERSION CHANGES
	git commit -m "Added VERSION and CHANGES files, Version bump to $version"

	cs_log "Create tag"
	git tag -a -m "Tagging version $version" "v$version"

	# log "Push tag"
	# git push origin --tags
fi

cs_succ "DONE. Created Release "$model_$version

popd &> /dev/null
