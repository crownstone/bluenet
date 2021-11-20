FROM balenalib/raspberrypi4-64-ubuntu

MAINTAINER Crownstone <ask@crownstone.rocks>

# Add support for armhf (32-bit arm binaries) required for nRF command line tools
RUN echo "deb [arch=armhf] http://ports.ubuntu.com/ focal main universe" >> /etc/apt/sources.list
RUN echo "deb-src [arch=armhf] http://ports.ubuntu.com/ focal main universe" >> /etc/apt/sources.list
RUN dpkg --add-architecture armhf

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends git cmake wget unzip python3 python3-pip libusb-1.0-0 make libsm6 && python3 -m pip install -U pip --user

# The 32-bit ARM dependencies of nRF command line tools
RUN apt-get install -y libc6:armhf libudev1:armhf libusb-1.0-0:armhf

# To bust the cache, pick a new number (different from 1)
# To pull from a local git repository use --build-arg 'git://localhost/' (note the trailing /).
ARG CACHEBUST=1
ARG BLUENET_BRANCH=master
ARG GITHUB_REPOS=https://github.com/crownstone/bluenet
RUN git clone -b $BLUENET_BRANCH $GITHUB_REPOS bluenet

RUN mkdir -p bluenet/build

ARG COMPILER_VERSION=10.3-2021.10
RUN cd bluenet/build && cmake .. -DSUPERUSER_SWITCH="" -DOVERWRITE_COMPILER_VERSION=$COMPILER_VERSION && make -j
