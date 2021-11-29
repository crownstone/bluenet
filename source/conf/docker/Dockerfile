FROM ubuntu:18.04

MAINTAINER Crownstone <ask@crownstone.rocks>

RUN apt-get update && apt-get install -y git cmake wget python3 python3-pip libusb-1.0-0 && python3 -m pip install -U pip --user

RUN git clone https://github.com/crownstone/bluenet

RUN mkdir -p bluenet/build

RUN cd bluenet/build && cmake .. -DSUPERUSER_SWITCH="" && make
