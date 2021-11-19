FROM balenalib/raspberrypi4-64-ubuntu

MAINTAINER Crownstone <ask@crownstone.rocks>

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends git cmake wget python3 python3-pip libusb-1.0-0 make && python3 -m pip install -U pip --user

RUN git clone https://github.com/crownstone/bluenet

# If pulling from local git repository enable the following (see README.md)
#ARG CACHEBUST=1
#RUN git clone -b master git://localhost/ bluenet

RUN mkdir -p bluenet/build

RUN cd bluenet/build && cmake .. -DSUPERUSER_SWITCH="" && make
