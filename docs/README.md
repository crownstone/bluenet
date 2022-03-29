# Bluenet documentation
Before reading the Bluenet documentation, make sure to have read the [root level README](https://github.com/crownstone/bluenet#readme) of the Bluenet GitHub repository.

Documentation for the Bluenet codebase is found in the Markdown pages in this `docs` folder. Below is an attempt to give some structure to this documentation. In addition, at the bottom a dictionary of often-used terms and acronyms is provided.

## Table of Contents
- [Bluetooth LE](#bluetooth-le)
- [Localization](#localization)
- [Power sensing](#power-sensing)
- [Dimming](#dimming)
- [UART communication](#uart-communication)
- [Miscellaneous functions](#miscellaneous-functions)
- [Development tools and resources](#development-tools-and-resources)
- [Development process](#development-process)
- [Deprecated pages](#deprecated-pages)
- [Dictionary and Acronyms](#dictionary-and-acronyms)

## Bluetooth LE
Crownstones are equipped with Bluetooth LE to communicate with each other, (smart)phones and other peripherals. The documentation covers both the Bluetooth LE protocol and the Bluenet protocol which runs on top of it. In addition, asset filtering is covered which is a term for filtering Bluetooth LE devices to get only the desired devices.

|Category|File name|Description|
|----|----|----|
|Bluetooth LE layer|||
||[BLUETOOTH](BLUETOOTH.md)|Describes the Bluetooth LE protocol, including the structure of Bluetooth LE packets for both connected and connectionless messages.|
||[MESH](MESH.md)|Describes the Bluetooth LE mesh protocol, including mesh packet structure.|
|Bluenet layer|||
||[PROTOCOL](protocol/PROTOCOL.md)|Describes the Bluenet protocol on top of the Bluetooth LE protocol in [BLUETOOTH](BLUETOOTH.md). It covers encryption, advertisements, services, and data structures of Bluenet Bluetooth LE communication.|
||[BROADCAST_PROTOCOL](protocol/BROADCAST_PROTOCOL.md)|Describes the Bluenet protocol specifically for (background and command) broadcasts from phone to Crownstones.|
||[MESH_PROTOCOL](protocol/MESH_PROTOCOL.md)|Describes the types of Bluenet mesh packets and their structure.|
||[SERVICE_DATA](protocol/SERVICE_DATA.md)|Describes the types of service data packets and their structure. The service data contains the state of the Crownstone which is broadcasted over advertisements (see [PROTOCOL](protocol/PROTOCOL.md))|
|Asset filtering|||
||[ASSET_FILTERING](protocol/ASSET_FILTERING.md)|Describes the methods of filtering advertisements to get only those of devices of interest (assets).|
||[CUCKOO_FILTER](protocol/CUCKOO_FILTER.md)|Describes asset filtering by the use of Cuckoo filters.|

## Localization
Through the received signal strength of Bluetooth LE transmissions by the Crownstones, smartphones can (after calibration) localize themselves within the Crownstone sphere.

|Category|File name|Description|
|----|----|----|
|Localization|||
||[LOCALIZATION](LOCALIZATION.md)|Describes iBeacon and Eddystone advertisement packages and their advantages and disadvantages in using them for user localization with Crownstones.|

## Power sensing
Crownstones measure voltages and currents of the wall sockets where they are installed. These properties can be used for e.g. appliance identification. The voltage and current sensors are read out by the processor via an ADC (SAADC).

|Category|File name|Description|
|----|----|----|
|Power sensing|||
||[ADC](ADC.md)|Describes the operation and some issues with reading out the ADC on the Crownstones.|

## Dimming
One of the functionalities of the Crownstone is the ability to dim lights. This is achieves by quickly switching off and on the AC power at the outlet side of the Crownstone. 

|Category|File name|Description|
|----|----|----|
|Dimming|||
||[DIMMER](DIMMER.md)|Describes the dimmer design and compatibility with dimmable light bulbs.|

## UART communication
The Crownstone has UART capability which can be used to communicate with a Crownstone as an alternative communication method to Bluetooth LE, e.g. for logging or debugging.
|Category|File name|Description|
|----|----|----|
|UART protocol|||
||[UART_PROTOCOL](protocol/UART_PROTOCOL.md)|Describes the types of UART messages and their structure.|

## Miscellaneous functions
Some of the more internal functions of Bluenet are gathered here. This includes low-level functions such as the bootloader, IPC, memory and logging but also high-level functions such as behaviour, setup, security and safety. Also included are microapps, which are an application layer on top of Bluenet.
|Category|File name|Description|
|----|----|----|
|Internal processes|||
||[IPC](development_environment/IPC.md)|Describes the RAM allocation protocol for inter-process communication (IPC). |
||[BOOTLOADER](BOOTLOADER.md)|Describes the two possible bootloaders for the Crownstone, the 'secure bootloader' and the 'mesh bootloader'.|
||[MEMORY](MEMORY.md)|Documents memory layout and decisions regarding memory on the Crownstones.|
||[FIRMWARE_SPECS](FIRMWARE_SPECS.md)|Describes various firmware features of Bluenet.|
||[LOGGING](LOGGING.md)|Describes logging methods of Bluenet, over UART and RTT.|
||[BEHAVIOUR](protocol/BEHAVIOUR.md)|Describes the methods of setting and retrieving behaviour on the Crownstone.|
||[SETUP](SETUP.md)|Describes the functionality of the Crownstone while in setup mode.|
|Safety and security|||
||[SECURITY](development_environment/SECURITY.md)|Describes security features of Bluenet.|
||[SOFTFUSES](SOFTFUSES.md)|Describes software-implemented safety features of Bluenet.|
|Microapp|||
||[MICROAPP](MICROAPP.md)|Documents the microapp functionality of bluenet, which is a way of running Arduino-like applications on top of Bluenet.|
||[INTRO_MICROAPP](tutorials/INTRO_MICROAPP.md)|Tutorial for getting started with microapps.|

## Development tools and resources
Getting started with Bluenet requires getting your hands dirty with the Crownstone hardware. The documentation related to working with the Crownstone hardware, including install guide and some useful tips, tricks and tool documentation for developers is gathered here.

As for hardware, at the core of the Crownstones lies the [nRF52832](https://www.nordicsemi.com/products/nrf52832) SoC by [Nordic Semiconductors](https://www.nordicsemi.com). The [PCA10040 development board](https://www.nordicsemi.com/Products/Development-hardware/nRF52-DK) can be used to test without using the (high-power!) Crownstones themselves.
Nordic also has a [SDK](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK) for working with the nRF5 SoCs.

|Category|File name|Description|
|----|----|----|
|Getting started|||
||[INSTALL](INSTALL.md)|Describes how to install and get started with Bluenet firmware. The main document for getting started.|
|Hardware|||
||[HARDWARE_VERSION](HARDWARE_VERSION.md)|Describes how to read the hardware version of the Crownstone from the device information service.|
||[RESOURCE_ALLOCATION](RESOURCE_ALLOCATION.md)|Describes which pins of the nRF52832 are currently used for which function.|
|Developer tools and guides|||
||[FOR_DEVS_UPGRADE_TO_SDK15](FOR_DEVS_UPGRADE_TO_SDK15.md)|Describes how to migrate to the [Nordic SDK15](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK).|
||[BUILD_SYSTEM](development_environment/BUILD_SYSTEM.md)|Describes the CMake build system of Bluenet.|
||[COLLECT_DEBUG_DATA_AT_HOME](development_environment/COLLECT_DEBUG_DATA_AT_HOME.md)|A guide for debugging a production Crownstone system.|
||[NORDIC_TOOLS](NORDIC_TOOLS.md)|Describes how to install Nordic tools such as `nrfconnect-core` and `nrfconnect-programmer` which allows for programming the nRF52832.|
||[NRF_ERROR_CODES](NRF_ERROR_CODES.md)|Combines NRF error codes for developer convenience.|
||[IDE](development_environment/IDE.md)|Describes CMake settings for working with Bluenet in the CLion IDE.|

## Development process
Some documentation pages are aimed at contributors to the Bluenet firmware and describe for example conventions on code style and naming, the release process, and testing.
|Category|File name|Description|
|----|----|----|
|Conventions|||
||[DEVELOPER_GUIDE](development_environment/DEVELOPER_GUIDE.md)|Describes guidelines on dynamic memory allocation and libraries.|
||[CODE_STYLE](development_environment/CODE_STYLE.md)|Describes code style conventions, including naming and commenting.|
||[PRODUCT_NAMING](PRODUCT_NAMING.md)|Describes product naming protocol.|
|Firmware releases|||
||[RELEASE_PROCESS](RELEASE_PROCESS.md)|Describes the release process for the Bluenet application.|
||[RELEASE_PROCESS_BOOTLOADER](RELEASE_PROCESS_BOOTLOADER.md)|Describes the release process for the bootloader.|
||[FACTORY_IMAGES](FACTORY_IMAGES.md)|Describes the process of generating factory images of releases for each type of hardware.|
|Testing|||
||[TESTING](TESTING.md)|Describes tests that need to be performed for each new Bluenet release.|
||[MEMORY_USAGE_TEST](MEMORY_USAGE_TEST.md)|Describes a test which tests how much memory the firmware uses.|
||[POWER_PROFILING](POWER_PROFILING.md)|Describes how to measure the power use of the nRF52832.|
|General|||
||[CHOICES](CHOICES.md)|Documents some past choices for Bluenet architecture and implementation.|
||[FAQ](FAQ.md)|Answers a limited amount of questions about the Bluenet architecture and implementation.|
||[TODO](TODO.md)|A list of todos for the Bluenet developers.|
|Tutorials|||
||[ADD_BOARD](ADD_BOARD.md)|Tutorial on how to adapt Bluenet when using a new board, such as a new development board or a new Crownstone version.|
||[ADD_NEW_COMMAND](tutorials/ADD_NEW_COMMAND.md)|Tutorial on how to add a new command to the command types specified in [PROTOCOL](protocol/PROTOCOL.md).|
||[ADD_BUILD_SUPPORT](tutorials/ADD_BUILD_SUPPORT.md)|Tutorial on how to build Bluenet code after (or before) developing new functionality.|
||[ADD_CONFIGURATION_OPTION](tutorials/ADD_CONFIGURATION_OPTION.md)|Tutorial on how to add a configuration option to the Bluenet code.|

## Deprecated pages
Finally, some of the documentation pages that are deprecated are gathered.
|Category|File name|Description|
|----|----|----|
|Deprecated|||
||[SERVICE_DATA_LEGACY](./protocol/SERVICE_DATA_LEGACY.md)|Replaced by the new [SERVICE_DATA](protocol/SERVICE_DATA.md) page.|

## Dictionary and Acronyms
|Term|Description|
|----|-----------|
|Advertisement (Bluetooth LE)|A broadcasted Bluetooth LE message. In the context of Bluenet, used for broadcasts by Crownstones.|
|Behaviour (Crownstone)|Configuration of the Crownstone which describes user-level functionality.|
|Bootloader (nRF5)|Piece of firmware on the nRF5 SoCs which is used for updating firmware and booting into an application.|
|BLE / Bluetooth LE | Bluetooth Low Energy. [More information](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy).|
|Characteristic (Bluetooth LE)|A container for a piece of data (e.g. a temperature value) sent over a BLE connection.|
|Crownstone|A device on which Bluenet runs. [More information](https://crownstone.rocks).|
|Cuckoo filter|A filter for filtering out uninteresting BLE devices. [More information](https://en.wikipedia.org/wiki/Cuckoo_filter).|
|DFU|Device Firmware Update.|
|iBeacon|A protocol designed by Apple for Bluetooth LE advertisements generally used in Bluenet for user localization.|
|IPC|Inter-process communication. The way for different processes (bootloader, application, microapp) to communicate with each other. [More information](development_environment/IPC.md).|
|J-Link|A debug probe that can be used with the nRF52832. [More information](https://www.segger.com/products/debug-probes/j-link/).|
|Mesh (Bluetooth LE)|A Bluetooth LE standard which allows for many-to-many communication. [More information](https://en.wikipedia.org/wiki/Bluetooth_mesh_networking).|
|Microapp|An application layer on top of Bluenet. [More information](MICROAPP.md).|
|nRF52832|SoC developed by Nordic Semiconductors used in Crownstone devices, on which Bluenet runs.|
|nRF52 DK|Development board by Nordic Semiconductors for the nRF52 SoCs. [More information](https://www.nordicsemi.com/Products/Development-hardware/nrf52-dk).|
|PCA10040|A specific version of the nRF52 DK with the nRF52832 SoC.|
|RSSI|Received Signal Strength Indicator.|
|RTT|Real-Time Transfer, used for logging. [More information](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/).|
|SAADC|Successive-Approximation Analog-to-Digital Converter, an ADC on the nRF52832.|
|Service (Bluetooth LE)|A container for data sent over a BLE connection.|
|SoftDevice (nRF5)|A piece of firmware by Nordic Semiconductors on nRF5 SoCs which controls BLE. Various versions exist.|
|UUID (Bluetooth LE)|Universally Unique ID. Defines the type of service in Bluetooth LE.|

