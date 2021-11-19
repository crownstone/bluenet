FROM balenalib/rpi-raspbian

MAINTAINER Crownstone <ask@crownstone.rocks>

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends git cmake wget python3 python3-pip libusb-1.0-0 make && python3 -m pip install -U pip --user

RUN git clone https://github.com/crownstone/bluenet

RUN mkdir -p bluenet/build

RUN cd bluenet/build && cmake .. -DSUPERUSER_SWITCH="" && make
