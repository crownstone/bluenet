# Bluenet

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](http://www.gnu.org/licenses/lgpl-3.0)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

Bluenet is the code running on the [Crownstone](http://crownstone.rocks) and GuideStone. Each Crownstone or GuideStone is a node in a network that uses signal strength for indoor localization of smartphones and wearables.


### Features
- Made for indoor localization.
- Includes a mesh network.
- Power measuring.
- iBeacon compliant.
- Configurable over the air.


# Usage
To use a device with bluenet code on it, you only need to know the Bluetooth protocol that bluenet uses. You can find the protocol definitions in the [protocol document](docs/PROTOCOL.md).

### Libraries
We also provide libraries which take care of the protocol:

- [Android](https://github.com/crownstone/bluenet-lib-android)
- [~~JavaScript~~](https://github.com/crownstone/bluenet-lib-js)
- [Python](https://github.com/crownstone/bluenet-lib-python)


# Build
To build the bluenet code yourself, follow the [installation instructions](docs/INSTALL.md).

### Bootloader
The Crownstones and GuideStones come with a bootloader, which enables over the air updates. This bootloader is a small adaptation of the one provided by Nordic. The build instructions are described on the [bootloader repository](https://github.com/crownstone/nrf51-dfu-bootloader-for-gcc-compiler).


# Communication
You can best communicate bugs and feature requests via the [issue tracker](https://github.com/crownstone/bluenet/issues). For all other questions, please, feel free to ask us anything on our [website](http://crownstone.rocks).


# Resources
The different other software tools and online resources have been mentioned above, but here is a short list:

- [Bluenet (this repos)](https://github.com/crownstone/bluenet)
- [Bluenet documentation](http://crownstone.github.io/bluenet/)
- [Bluenet documentation branch](https://github.com/crownstone/bluenet/tree/gh-pages)
- [Android library](https://github.com/crownstone/bluenet-lib-android)
- [~~JavaScript library~~](https://github.com/crownstone/bluenet-lib-js)
- [Python library](https://github.com/crownstone/bluenet-lib-python)
- [Bootloader](https://github.com/crownstone/nrf51-dfu-bootloader-for-gcc-compiler)
- [Crownstone website](http://crownstone.rocks)
- [Crownstone Android app](https://play.google.com/store/apps/details?id=rocks.crownstone.consumerapp)
- [Crownstone app source](https://github.com/crownstone/crownstone-app)
- [Crownstone USB image](https://github.com/crownstone/crownstone-image)


# Commercial use
This code is used in a commercial product, the [Crownstone](http://crownstone.rocks). Our intellectual property exists on two levels. First, you can license our technology to create these extremely cheap BLE building blocks yourself. Second, we build services around BLE-enabled devices. This ranges from smartphones to [gadgets such as the "virtual memo"](http://crownstone.nl/2014/07/15/ble-dobeacon-a-virtual-memo/). What this means for you as a developer is that we can be transparent about the software on the Crownstone, which is why this repository exists. Feel free to build your own services on top of it, and benefit from our software development as much as you want. It would be much appreciated to if you mention us in your project.

If you want to buy our hardware, please navigate to our [website](http://crownstone.rocks). We offer a
hardware development kit for professional use.


# Copyrights

The copyrights (2014-2016) belongs to the team of Crownstone B.V. and are provided under an noncontagious open-source license:

* Authors: Dominik Egger, Bart van Vliet, Anne van Rossum, Marc Hulscher, Peet van Tooren, Alex de Mulder, Christian Haas
* Date: 27 Jan. 2014
* Triple-licensed: LGPL v3+, Apache, MIT
* Crownstone B.V., http://crownstone.rocks
* Rotterdam, The Netherlands

This code is built on the shoulders of giants. Our special thanks go to Christopher Mason for the initial C++ code base at [http://hg.cmason.com/nrf](http://hg.cmason.com/nrf) and Trond Einar Snekvik, department of Engineering Cybernetics at Norwegian University of Science and Technology (and Nordic Semiconductors) for the meshing functionality ([OpenMesh](https://github.com/NordicSemiconductor/nRF51-ble-bcast-mesh)).
