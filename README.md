# Indoor Localization with BLE

This project aims at a wireless network with BLE nodes that use their mutual signal strengths to build up a network with their relative locations. This can be used later by someone carrying a smartphone to establish their location indoors. Everybody say that they can do it, but very few solutions are actually out there. Let's hope we can change that.

Bluetooth LE (BLE) does not inherently fit a wireless network. We have technology in-house (at the Almende group) that can do this [https://en.wikipedia.org/wiki/MyriaNed](Myrianed), but it has not been accepted in the mainstream yet. By the way, it is my personal opinion that solutions such as ZigBee, Z-Wave, MyriaNed, and other mesh solutions, will remain marginal except if they get accepted in a common handheld.

That's why BLE is interesting. A lot of phones come with BLE, so a solution is automatically useful to a large variety of people. It is not the best technology for the job. The network topology is a Personal Area Network (PAN), not a Local Area Network (LAN). This means that you cannot have all nodes communicating with all other nodes at the same time. To get RSSI values we will have to set up connections to other nodes and tear them down again. Not very efficient. But it will do the job. The new SoftDevice from Nordic (the S120 instead of the S110) might change this, but it is still in its alpha stage. So, for now we consider a network with known nodes, setting up connections amongst them will be relatively easy. On the moment I am not concerned with security so establishing some zeroconf method to detect which nodes belong to the network will not be part of this codebase yet. My idea however is to do this through synchronizing over the main switch in a home. Turn everything off and the network starts searching for a smartphone who is advertising authentication to use. When this is set, it will be used next time the power goes down. 

Feel free to clone this repos.

The code base comes from [http://hg.cmason.com/nrf](http://hg.cmason.com/nrf). Thanks a lot Christopher!

## Installation

The installation should not be hard when you have the Nordic SDK. Get this from their website after buying a development kit. You also need a cross-compiler for ARM. You need the JLink utilities from Segger. And you need cmake for the build process.

* [Nordic nRF51822 SDK](https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822)
* [Nordic S110 Softdevice](http://www.nordicsemi.com/eng/Products/S110-SoftDevice-v7.0)
* [JLink Software](http://www.segger.com/jlink-software.html)
* sudo aptitude install cmake

Fork the code by clicking on:

* Fork [https://github.com/mrquincle/bluenet/fork](https://github.com/mrquincle/bluenet/fork).
* `git clone https://github.com/${YOUR_GITHUB_USERNAME}/bluenet`
* let us call this directory $BLUENET

Now you will have to set all fields in the configuration file:

* cp CMakeBuild.config.default CMakeBuild.config
* adjust the `NRF51822_DIR` to wherever you installed the Nordic SDK (it should have `/Include` and `/Source` subdirectories
* adjust the `SOFTDEVICE_DIR` to wherever you unzipped the latest SoftDevice from Nordic
* adjust the type `SOFTDEVICE` accordingly (basename of file without `_softdevice.hex`)
* adjust the `COMPILER_PATH` and `COMPILER_TYPE` to your compiler (it will be used as `$COMPILER_PATH\bin\$COMPILER_TYPE-gcc`)
* adjust `JLINK` to the full name of the JLink utility (JLinkExe on Linux)
* adjust `JLINK_GDB_SERVER` to the full name of the JLink utility that supports gdb (JLinkGDBServer on Linux)

Let us now install the SoftDevice on the nRF51822:

* cd scripts
* ./softdevice.sh build
* ./softdevice.sh upload

Now we can build our own software:

* cd $BLUENET
* make

And we can upload it:

* cd scripts
* this would do the same as building above ./crownstone.sh build 
* ./crownstone.sh upload
* ./crownstone.sh debug

And there you go. There are some more utility scripts, such as `reboot.sh`. Use as you wish. 

## Todo list

* Clean up code
* Obtain RSSI values
* Set up management for establishing connections, getting RSSI values, and tear down connections again
* Create an algorithm to come up with all locations of the nodes. These positions are relative, not absolute. The network will be known up to rotation and scale.
* Implement this algorithm in a distributed fashion. The type of algorithm I have in mind is belief propagation / message passing.

## Copyrights

Obviously, the copyrights of the code written by Christoper, belong to him.

The copyrights (2014) for the rest of the code belongs to me and are provided under an noncontagious open-source license:

* Author: Anne van Rossum
* Date: 27 Jan. 2014
* License: LGPL v3
* Almende B.V., http://www.almende.com and DoBots B.V., http://www.dobots.nl
* Rotterdam, The Netherlands

