# Indoor Localization with BLE

This project aims at a wireless network with BLE nodes that use their mutual signal strengths to build up a network with their relative locations. This can be used later by someone carrying a smartphone to establish their location indoors. Everybody say that they can do it, but very few solutions are actually out there. Let's hope we can change that.

# Communication

You can best communicate bugs and feature requests via the [issue tracker](https://github.com/dobots/bluenet/issues). For all other questions, please, feel free to ask us anything on [muut](https://muut.com/dobots).

# Why Bluetooth LE?

Bluetooth LE (BLE) does not inherently fit a wireless network. We have technology in-house (at the Almende group) that can do this, called [Myrianed](https://en.wikipedia.org/wiki/MyriaNed), but dedicated solutions are difficult to be accepted in the mainstream. This is also the case for ZigBee, Z-Wave, and other mesh solutions. They will remain marginal unless they get accepted in a common handheld. BLE is not just a random technology, it is of paramount importance to understand this.

That's why BLE is interesting. A lot of phones come with BLE, so a solution is automatically useful to a large variety of people. It is not the best technology for the job. The network topology is a Personal Area Network (PAN), not a Local Area Network (LAN). This means that you cannot have all nodes communicating with all other nodes at the same time. To get RSSI values we will have to set up connections to other nodes and tear them down again. Not very efficient. But it will do the job.

However, through the Timeslot API, it is possible to run a totally different protocol parallel to BLE using the same radio. This means that we can have a mesh network at the same time as providing for BLE functionality. The new S130 SoftDevice adds to that even multiple BLE roles at the same time. So many possibilities arise!

Feel free to clone this repos.

The code base comes from [http://hg.cmason.com/nrf](http://hg.cmason.com/nrf). Thanks a lot Christopher!

## Installation

For installation and configuration, see the [installation instructions](https://github.com/dobots/bluenet/blob/master/INSTALL.md).

## Todo

Things to do are listed in our [todo list](https://github.com/dobots/bluenet/blob/master/TODO.md).

## Commercial use

This code is used in a commercial product at DoBots, the [Crownstone](http://dobots.nl/products/crownstone). Our intellectual property exists on two levels. First, you can license our technology to create these extremely cheap BLE building blocks yourself. Second, we build services around BLE-enabled devices. This ranges from smartphones to [gadgets such as the "virtual memo"](http://dobots.nl/2014/07/15/ble-dobeacon-a-virtual-memo/). What this means for you as a developer is that we can be transparent about the software on the Crownstone, which is why this repository exists. Feel free to build your own services on top of it, and benefit from our software development as much as you want.

It would be much appreciated to state "DoBots inside" in which case we will be happy to provide support to your organisation.

If you want to buy our hardware, please navigate to our [website](http://dobots.nl/products/crownstone). We offer a
hardware development kit for professional use.

## Resources

The different other software tools and online resources have been mentioned above, but here is a short list:

* [Bluenet (this repos)](https://github.com/dobots/bluenet)
* [Bluenet documentation](http://dobots.github.io/bluenet/)
* [Bluenet documentation branch](https://github.com/dobots/bluenet/tree/gh-pages)
* [Bootloader](https://github.com/dobots/nrf51-dfu-bootloader-for-gcc-compiler/tree/s110)
* [DFU upload tool](https://github.com/dobots/nrf51_dfu_linux)
* [Crownstone website](http://dobots.nl/products/crownstone)
* [Crownstone Android app](https://play.google.com/store/apps/details?id=nl.dobots.CrownStone)
* [Crownstone app source](https://github.com/dobots/crownstone-app)
* [Crownstone USB image](https://github.com/dobots/crownstone-image)
* [Crownstone SDK manual](https://docs.google.com/a/almende.org/document/d/1uJ83c0rLC_swX-cVFDl0UyIFc9_81ccfRLK2vJJF1VE/edit)

If you have any problems that are not solved by documentation, please ask them as a GitHub [issue](https://github.com/dobots/bluenet/issues).
In that case everybody can profit from things that are currently incorrectly formulated, not yet formulated, or need
some other attention from us developers.

## Copyrights

Obviously, the copyrights of the code written by Christopher, belong to him.

The copyrights (2014-2015) for the rest of the code belongs to the team of Distributed Organisms B.V. and are provided under an noncontagious open-source license:

* Authors: Anne van Rossum, Dominik Egger, Bart van Vliet, Marc Hulscher, Peet van Tooren, Christian Haas
* Date: 27 Jan. 2014
* License: LGPL v3+, Apache, or MIT, your choice
* Almende B.V., http://www.almende.com and DoBots B.V., http://www.dobots.nl
* Rotterdam, The Netherlands

Note, that we do not use any header files from Nordic. The header files are rewritten from scratch by Christopher
especially for that purpose. This means you are not finding a Nordic license text in this repository.
Of course, this means that you will have to get those files from Nordic via different means (we recommend to buy the
development kit). We just didn't want to "contaminate" this repository with files that we don't understand the license
implications of.

The only exception is the code for the meshing functionality that is put online by Nordic itself on
https://github.com/NordicSemiconductor/nRF51-ble-bcast-mesh. This functionality can be found in `src/protocol` and
`include/protocol/` and falls of course under the Nordic license. You can disable the meshing functionality with
`CHAR_MESHING=0` if you do want to exclude that code from becoming part of the binary. You can still use the services
for individual nodes, but they won't be able to communicate with each other in that case.

