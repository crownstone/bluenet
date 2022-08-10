# A docker file with Ubuntu plus build dependencies to build bluenet

FROM ubuntu:20.04

MAINTAINER Crownstone <ask@crownstone.rocks>

RUN DEBIAN_FRONTEND=noninteractive TZ=Europe/Amsterdam apt-get update
RUN DEBIAN_FRONTEND=noninteractive TZ=Europe/Amsterdam apt-get install -y git cmake wget unzip python3 python3-pip libusb-1.0-0 npm && python3 -m pip install -U pip --user

# Install wheel
RUN python3 -m pip -q install wheel

# Install nrfutil
RUN python3 -m pip -q install nrfutil

# Clone bluenet to get easy access to installation scripts
# Get specific branch for this docker
RUN git clone -b maintain/build-system-refactor https://github.com/crownstone/bluenet

# Install cross-compiler
RUN cd bluenet/scripts/download && ./download-ubuntu-gcc-arm-none-eabi.sh ../..

# Install JLink
RUN cd bluenet/scripts/download && SUPERUSER_SWITCH= ./download-ubuntu-jlink.sh ../..

# Install nrfjprog
RUN cd bluenet/scripts/download && SUPERUSER_SWITCH= ./download-ubuntu-nrfjprog.sh ../..

# Install bluenet-logs
# RUN python3 -m pip install bluenet-logs

# Install Crownstone python libs
# RUN python3 -m pip install crownstone-ble crownstone-uart crownstone-cloud

# Install nrfconnect-core
# RUN cd bluenet/scripts/download && ./download-ubuntu-nrfconnect-core.sh ../..

# Install nrfconnect-programmer
# RUN cd bluenet/scripts/download && ./download-ubuntu-nrfconnect-programmer.sh ../..

# Set default branch to master
RUN cd bluenet && git checkout master

