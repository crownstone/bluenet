# Bluetooth Low Energy

Bluetooth Low Energy or Bluetooth LE (the Bluetooth SIG group does not want you to use BLE as an abbreviation) is
supported on most of the modern smartphones. Bluetooth LE is quite different from legacy Bluetooth. We will describe
the aspects to it that are important for hardware running the bluenet firmware.

There are seven ways the Bluetooth LE radio is used in bluenet.

1. **Receive connection commands**. To get commands from a smartphone (or hub) through a connection, see the [connection protocol](PROTOCOL.md).
2. **Receive connectionless commands**. To receive (mostly by smartphone) broadcasted Bluetooth LE advertisements, see the [broadcast protocol](BROADCAST_PROTOCOL.md).
3. **Receive connectionless presence**. To receive any type of Bluetooth LE advertisement, for in-network presence detection and in-network localization (not yet documented).
4. **Send connectionless state**. To broadcast data from bluenet about the state of the device towards smartphones etc., see the [service data protocol](SERVICE_DATA.md).
5. **Send connectionless presence**. To broadcast presence for indoor localization in the form of iBeacon messages, see [localization](LOCALIZATION.md).
6. **Send and receive over mesh**. To communicate with other bluenet devices using Bluetooth Mesh, see the [mesh overview](MESH.md) and [mesh protocol](MESH_PROTOCOL.md) documents.
7. **Send connection commands**. To set up a connection to other Crownstones or other Bluetooth LE devices (not yet documented).

## Receive connection commands

It is possible to send some commands in a connectionless manner through encrypted advertisements which are broadcasted from a smartphone (see next section). However, it is hard to have this working properly on iOS in the background. Hence, we require connections to send on, off, dim, and other commands to the device running bluenet.

A smartphone when setting up a connection has to perform a service discovery process. Bluetooth LE services and
characteristics are obtained from the bluenet device. This process takes a while, a couple of seconds. The reason for this is that it is not truly possible to reliably cache the services and characteristics properly across all devices. If
the device running bluenet that you want to connect to is not in the proximity of the smartphone, the connection process can take quite some while or fail altogether.

For this, the so-called [ConstellationAPI](https://github.com/crownstone/crownstone-app/blob/master/docs/bleTasks/ConstellationAPI.MD) has been implemented. It sorts bluenet devices on their proximity and is able to set up a connection to a couple of them in parallel. The sessions are time-limited. You can send a series of different dim commands while using the dim slider without connecting and disconnecting within a certain time period. 

There are no permanent connections. This might be something for the future to further remove delays.

## Receive connectionless commands

TBD

## Receive connectionless presence

TBD

## Send connectionless state

TBD

## Send connectionless presence

TBD

## Send and receive over mesh

TBD

## Send connection commands

TBD

