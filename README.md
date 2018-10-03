# Bluenet

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](http://www.gnu.org/licenses/lgpl-3.0)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Build Status](https://travis-ci.org/crownstone/bluenet.svg?branch=master)](https://travis-ci.org/crownstone/bluenet)

Bluenet is code running on [Crownstone](http://crownstone.rocks) compatible devices such as [electronic connectors](https://shop.crownstone.rocks/products/built-in-crownstone), [electronic EU plugs](https://shop.crownstone.rocks/products/crownstone-plugs) and [grid-powered beacons, GuideStones](https://shop.crownstone.rocks/products/guidestones). Each Crownstone or GuideStone is a node in a network that uses signal strength for indoor localization of smartphones and wearables.


### Features
- Made for indoor localization.
- Includes a mesh network.
- Power measuring.
- iBeacon compliant.
- Configurable over the air.

# Usage
To use a device with bluenet code on it, you only need to know the Bluetooth protocol that bluenet uses. You can find the protocol definitions in the [protocol document](docs/PROTOCOL.md).

### Libraries
We also provide libraries which implements this protocol (so if you build a smartphone app you will not be bogged down into Bluetooth Low Energy details):

- [Android](https://github.com/crownstone/bluenet-lib-android)
- [iOS](https://github.com/crownstone/bluenet-ios-lib)
- [Python](https://github.com/crownstone/bluenet-python-lib)


# Build
To build the bluenet code yourself, follow the [installation instructions](docs/INSTALL.md).

### Bootloader
The Crownstones and GuideStones come with a bootloader, which enables over the air updates. This bootloader is a small adaptation of the one provided by Nordic. The build instructions are described on the [bootloader repository](https://github.com/crownstone/nrf51-dfu-bootloader-for-gcc-compiler).

# Communication
You can best communicate bugs and feature requests via the [issue tracker](https://github.com/crownstone/bluenet/issues). For all other questions, please, feel free to ask us anything on our [website](http://crownstone.rocks).

# Resources
The different other software tools and online resources have been mentioned above, but here is a short list:

| Resource                                                                                              | Description                                                      |
| ---                                                                                                   | ---                                                              |
| [Bluenet firmware](https://github.com/crownstone/bluenet)                                             | This repository, open-source firmware (C++) for smart plugs      |
| [Bluenet documentation](http://crownstone.github.io/bluenet/)                                         | Documentation of the firmware (doxygen-based)                    |
| [Android library](https://github.com/crownstone/bluenet-lib-android)                                  | Android library (Java)                                           |
| [iOS library](https://github.com/crownstone/bluenet-ios-lib)                                          | iOS library (Swift)                                              |
| [Bootloader](https://github.com/crownstone/nrf51-dfu-bootloader-for-gcc-compiler)                     | Bootloader, fork of Nordic's bootloader for the smart plugs      |
| [Crownstone website](http://crownstone.rocks)                                                         | Website and shop for the Crownstone plugs                        |
| [Crownstone Android app](https://play.google.com/store/apps/details?id=rocks.crownstone.consumerapp)  | Android app on the Play Store                                    |
| [Crownstone iOS app](https://itunes.apple.com/us/app/crownstone/id1136616106?mt=8)                    | iOS app on the Apple Store                                       |
| [Crownstone app source](https://github.com/crownstone/crownstone-app)                                 | Source code for the cross-platform app (React Native)            |


# Commercial use
This code is used in a commercial product, the [Crownstone](http://crownstone.rocks). Our intellectual property exists on two levels:

- The hardware is [patented](http://mijnoctrooi.rvo.nl/fo-eregister-view/search/details/1041053_NP/0/0/1/10/0/0/0/null_null/KG51bW1lcjooMTA0MTA1MykpIEFORCBwYXRlbnRSZWNvcmRTZXE6MQ==) under Dutch law with the main aim to protect you as a developer against fraudulent claims. 
- The software in these repositories allow developers to build a complete indoor localization system. We do have pro-versions of e.g. the basic [indoor localization library](https://github.com/crownstone/bluenet-ios-basic-localization) to be used by other companies under a commercial license.

Summarized, as a developer you can build your own services on top of the Crownstone stack. Benefit from our software development as much as you want! For PR reasons, it would be much appreciated to if you mention us of course!

# Commercial activities

Crownstone sells the Crownstone products through our [own channels](https://shop.crownstone.rocks). Buying our products is the best way in which you can support open-source projects like these! 

Crownstone also integrates their hardware technology in third-party products, ranging from lights to desks.

# Contributors

<!-- CONTRIBUTORS:START -->
| [<img src="https://avatars.githubusercontent.com/u/2011969" width="110px;"/><br /><sub>Bart van Vliet</sub>](https://github.com/vliedel) | [<img src="https://avatars.githubusercontent.com/u/2161587" width="110px;"/><br /><sub>Dominik Egger</sub>](https://github.com/eggerdo) | [<img src="https://avatars.githubusercontent.com/u/1428585" width="110px;"/><br /><sub>Anne van Rossum</sub>](https://github.com/mrquincle) | [<img src="https://avatars.githubusercontent.com/u/5363277" width="110px;"/><br /><sub>Alex de Mulder</sub>](https://github.com/AlexDM0) | [<img src="https://avatars.githubusercontent.com/u/4710354" width="110px;"/><br /><sub>Marc Hulscher</sub>](https://github.com/marciwi) | [<img src="https://avatars2.githubusercontent.com/u/10497648" width="110px;"/><br /><sub>Christian Haas</sub>](https://github.com/chaasfr) 
| :---: | :---: | :---: | :---: | :---: | :---: | 
| [<img src="https://avatars2.githubusercontent.com/u/1262780" width="110px;"/><br /><sub><strong>Peet van Tooren</strong></sub>](https://github.com/kurkesmurfer) |
<!-- CONTRIBUTORS:END -->

# Copyrights

The copyrights (2014-2017) belongs to the team of Crownstone B.V. and are provided under an noncontagious open-source license:

* Authors: Dominik Egger, Bart van Vliet, Anne van Rossum, Marc Hulscher, Peet van Tooren, Alex de Mulder, Christian Haas
* Date: 27 Jan. 2014
* Triple-licensed: LGPL v3+, Apache, MIT
* Crownstone B.V., http://crownstone.rocks
* Rotterdam, The Netherlands

This code is built on the shoulders of giants. Our special thanks go to Christopher Mason for the initial C++ code base at [http://hg.cmason.com/nrf](http://hg.cmason.com/nrf) and Trond Einar Snekvik, department of Engineering Cybernetics at Norwegian University of Science and Technology (and Nordic Semiconductor) for the meshing functionality ([OpenMesh](https://github.com/NordicSemiconductor/nRF51-ble-bcast-mesh)) and Nordic Semiconductor for the beautiful SoftDevices they have developed. The code of Mason falls under the same triple license. The code by Nordic falls under the license from Nordic (and that code is not part of this repository).
