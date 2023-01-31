FROM ghcr.io/crownstone/bluenet:latest

MAINTAINER Crownstone <ask@crownstone.rocks>

RUN cd bluenet && git pull

RUN mkdir -p bluenet/build

RUN cd bluenet/build && cmake .. && make
