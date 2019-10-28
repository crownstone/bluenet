FROM ubuntu:18.04

MAINTAINER Crownstone <ask@crownstone.rocks>

RUN apt-get update && apt-get install -y git cmake wget 

RUN git clone https://github.com/crownstone/bluenet 

RUN mkdir -p bluenet/build 

RUN cd bluenet/build && cmake .. -DSUPERUSER_SWITCH="" && make
