# Bluenet
Bluenet is the code running on the [Crownstone](http://crownstone.rocks) and GuideStone. Each Crownstone or GuideStone is a node in a network that uses signal strength for indoor localization of smartphones and wearables.


### Features
- Made for indoor localization.
- Includes a mesh network.
- Power measuring.
- iBeacon compliant.
- Configurable over the air.


# Usage
To use a device with bluenet code on it, you only need to know the Bluetooth protocol that bluenet uses. You can find the protocol definitions in the [protocol document](PROTOCOL.md).

### Libraries
We also provide libraries which take care of the protocol:

- [Android](https://github.com/dobots/bluenet-lib-android)
- [~~JavaScript~~](https://github.com/dobots/bluenet-lib-js)
- [Python](https://github.com/dobots/bluenet-lib-python)


# Build
To build the bluenet code yourself, follow the [installation instructions](INSTALL.md).

### Bootloader
The Crownstones and GuideStones come with a bootloader, which enables over the air updates. This bootloader is a small adaptation of the one provided by Nordic. The build instructions are described on the [bootloader repository](https://github.com/dobots/nrf51-dfu-bootloader-for-gcc-compiler).


# Communication
You can best communicate bugs and feature requests via the [issue tracker](https://github.com/dobots/bluenet/issues). For all other questions, please, feel free to ask us anything on our [website](http://crownstone.rocks).


# Resources
The different other software tools and online resources have been mentioned above, but here is a short list:

- [Bluenet (this repos)](https://github.com/dobots/bluenet)
- [Bluenet documentation](http://dobots.github.io/bluenet/)
- [Bluenet documentation branch](https://github.com/dobots/bluenet/tree/gh-pages)
- [Android library](https://github.com/dobots/bluenet-lib-android)
- [JavaScript library](https://github.com/dobots/bluenet-lib-js)
- [Python library](https://github.com/dobots/bluenet-lib-python)
- [Bootloader](https://github.com/dobots/nrf51-dfu-bootloader-for-gcc-compiler)
- [Crownstone website](http://crownstone.rocks)
- [Crownstone Android app](https://play.google.com/store/apps/details?id=nl.dobots.CrownStone)
- [Crownstone app source](https://github.com/dobots/crownstone-app)
- [Crownstone USB image](https://github.com/dobots/crownstone-image)


# Commercial use
This code is used in a commercial product, the [Crownstone](http://crownstone.rocks). Our intellectual property exists on two levels. First, you can license our technology to create these extremely cheap BLE building blocks yourself. Second, we build services around BLE-enabled devices. This ranges from smartphones to [gadgets such as the "virtual memo"](http://dobots.nl/2014/07/15/ble-dobeacon-a-virtual-memo/). What this means for you as a developer is that we can be transparent about the software on the Crownstone, which is why this repository exists. Feel free to build your own services on top of it, and benefit from our software development as much as you want. It would be much appreciated to if you mention us in your project.

If you want to buy our hardware, please navigate to our [website](http://crownstone.rocks). We offer a
hardware development kit for professional use.


# Copyrights
The code base comes from [http://hg.cmason.com/nrf](http://hg.cmason.com/nrf). Obviously, the copyrights of the code written by Christopher, belong to him.

For the meshing functionality we use [OpenMesh](https://github.com/NordicSemiconductor/nRF51-ble-bcast-mesh) written by Trond Einar Snekvik, department of Engineering Cybernetics at Norwegian University of Science and Technology (and Nordic Semiconductors).

The copyrights (2014-2015) for the rest of the code belongs to the team of Distributed Organisms B.V. and are provided under an noncontagious open-source license:

* Authors: Anne van Rossum, Dominik Egger, Bart van Vliet, Marc Hulscher, Peet van Tooren, Christian Haas
* Date: 27 Jan. 2014
* License: LGPL v3+, Apache, or MIT, your choice
* Almende B.V., http://www.almende.com and DoBots B.V., http://www.dobots.nl
* Rotterdam, The Netherlands
