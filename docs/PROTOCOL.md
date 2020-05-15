# Bluenet protocol v5.0.0
-------------------------

This only documents the latest protocol, older versions can be found in the git history.

# Index

- [Setup](#setup). How to setup the crownstone.
- [Encryption](#encryption). How to encrypt and decrypt the data.
- [Advertisements](#advertisement_data). What data is broadcasted by the crownstones.
- [Broadcast commands](#broadcasts). Broadcast commands.
- [Services and characteristics](#services). Which Bluetooth GATT services and characteristics the crownstones have.
- [Data structures](#data_structs). The data structures used for the characteristics, advertisements, and mesh.
    - [Control](#control_packet). Used to send commands to the Crownstone.
    - [Result](#result_packet). The result of a command.
    - [State](#state_types). State variables of the Crownstone.


<a name="setup"></a>
# Setup mode
When a Crownstone is new or factory reset, it will go into setup mode.

The setup process goes as follows:

- Crownstone is in setup mode ([Setup service](#setup_service) active).
- Phone connects to the Crownstone.
- Phone reads the **session key** from the [setup service](#setup_service).
- Phone reads the [session data](#session_data) from the [setup service](#setup_service).
- From here on, all writes and read are encrypted using the session key and data, as explained [here](#encrypted_write_read).
- Phone subscribes to [result](#setup_service) characteristic.
- Phone commands Crownstone to [setup](#control_packet) via the control characteristic.
- Phone waits for result to become SUCCESS (See [result packet](#result_packet)).
- Crownstone will reboot to normal mode.

<a name="normal"></a>
# Normal mode
When a Crownstone has been set up, it will run in "normal mode".

To write a command via connection, the process goes as follows:

- Crownstone is in normal mode ([Crownstone service](#crownstone_service) active).
- Phone connects to the Crownstone.
- Phone reads the [session data](#session_data) from the [Crownstone service](#crownstone_service).
- From here on, all writes and read are encrypted using the session key and data, as explained [here](#encrypted_write_read).
- Phone subscribes to [result](#crownstone_service) characteristic.
- Phone writes a [control command](#control_packet) to the [control characteristic](#crownstone_service).
- Phone waits for result, see [result packet](#result_packet).
- Next command can be written.


<a name="encryption"></a>
# Encryption
By default, Crownstones have encryption enabled as a security and privacy measure.

What is encrypted:

- The [service data](SERVICE_DATA.md) is encrypted using the service data key.
- Values that are **read from** the characteristics are encrypted unless specified differently.
- Values that are **written to** the characteristics are encrypted unless specified differently.
- Messages over the mesh.
- Broadcasted commands.


<a name="ecb_encryption"></a>
## AES 128 ECB encryption

Some packets are [ECB encrypted](https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Electronic_Codebook_.28ECB.29).


<a name="ctr_encryption"></a>
## AES 128 CTR encryption

We use the [AES 128 CTR](https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Counter_.28CTR.29) method to encrypt everything that is written to- and read from characteristics. For this you need an 8 byte number called a **nonce**. The counter starts at 0 for each packet, and is increased by 1 for every block of 16 bytes in the packet.

### Nonce

The nonce is a combination of 2 pieces: the session nonce and the packet nonce

![Nonce](../docs/diagrams/nonce.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8 [] | Packet nonce | 3 | Packet nonce, sent with every packet (see [encrypted packet](#encrypted_packet)). Should be different for each encrypted packet.
uint8 [] | Session nonce | 5 | Session nonce, should be [read](#session_data) when connected, each time you connect.

<a name="session_data"></a>
#### Session data

After connecting, you should first read the session data from the [Crownstone service](#crownstone_service).
The session data is [ECB encrypted](#ecb_encryption) with the basic key, or when in setup mode: the session key.
After decryption, you should verify whether you have read and decrypted succesfully by checking if the validation is equal to **0xCAFEBABE**.
The session nonce and validation key will be different each time you connect.

![Session data](../docs/diagrams/session-data.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 32 | Validation | 4 | 0xCAFEBABE as validation.
uint8 | Protocol | 1 | The protocol version to use for communication.
uint8 [] | Session nonce | 5 | The session nonce for this session. Used to encrypt or decrypt packets.
uint8 [] | Validation key | 4 | The validation key for this session. Used to verify decryption/encryption.
uint8 [] | Padding | 2 | Zero-padding so that the whole packet is 16 bytes.


<a name="encrypted_write_read"></a>
## Reading and writing characteristics

When reading and writing characteristics, the data is wrapped in an [encrypted packet](#encrypted_packet).

After subscribing, notified data will be sent as [multipart notifications](#multipart_notifaction_packet).

<a name="multipart_notifaction_packet"></a>
### Multipart notification packet

When data is sent via notifications, it will be done via multiple notification packets.

![Multipart notification packet](../docs/diagrams/multipart-notification-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Counter | 1 | Part counter: starts at 0, 255 for last packet.
uint8 [] | Data part |  | Part of the data.

Once you received the last packet, you should concatenate all data parts to get the payload (which is usually an [encrypted packet](#encrypted_packet)).

<a name="encrypted_packet"></a>
### Encrypted packet

Unlike the name suggests, only the payload of this packet is encrypted. The header is used to determine how to decrypt the payload.

![Encrypted packet](../docs/diagrams/encrypted-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8 [] | Packet nonce | 3 | First 3 bytes of nonce used for encrypting the payload, see [CTR encryption](#ctr_encryption).
uint8 | User level | 1 | 0: Admin, 1: Member, 2: Basic, 100: Setup. This determines which key has been used for encryption.
[Encrypted payload](#encrypted_payload) | Encrypted payload | N*16 | The encrypted payload of N blocks.

<a name="encrypted_payload"></a>
#### Encrypted payload

![Encrypted payload](../docs/diagrams/encrypted-payload.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint32 | Validation key | 4 | Should be equal to the read [validation key](#session_info).
uint8 | Payload |  | Whatever data would have been sent if encryption was disabled.
uint8 | Padding |  | Zero-padding so that the whole packet is of size N*16 bytes.






<a name="advertisement_data"></a>
# Advertisements
By default, [iBeacon advertisements](#ibeacon_adv_packet) will be broadcast  at a regular interval.
On top of that, [advertisements with service data](#service_data_adv_packet) are also broadcasted at regular interval. The [service data](SERVICE_DATA.md) contains useful info about the state of the Crownstone.

The iBeacon advertisements have a different MAC address, since these advertisements need to have the connectable flag to be unset.
The advertisements with service data will use the original MAC address, and will have the connectable flag set.
To calculate the MAC address used for iBeacon advertisements, simply subtract 1 from the first byte of the original MAC address (overflow only that byte, so it only changes the first byte).


<a name="ibeacon_adv_packet"></a>
### iBeacon advertisement packet
This packet is according to iBeacon spec, see for example [here](http://www.havlena.net/en/location-technologies/ibeacons-how-do-they-technically-work/).

![iBeacon advertisement packet](../docs/diagrams/advertisement-ibeacon-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | AD Length | 1 | Length of the next AD structure.
uint 8 | AD Type | 1 | 0x01: flags.
uint 8 | Flags | 1 |
uint 8 | AD Length | 1 | Length of the next AD structure.
uint 8 | AD Type | 1 | 0xFF: manufacturer specific data.
uint 8 | Company id | 2 | 0x004C: Apple.
uint 8 | iBeacon type | 1 | 0x02: iBeacon.
uint 8 | iBeacon length | 1 | iBeacon struct length (0x15).
uint 8 | Proximity UUID | 16 | Configurable number.
uint 16 | Major | 2 | Configurable number.
uint 16 | Minor | 2 | Configurable number.
int 8 | TX power | 1 | Received signal strength at 1 meter.

<a name="service_data_adv_packet"></a>
### Service data advertisement
This packet contains the state of the Crownstone.

![Service data advertisement](../docs/diagrams/advertisement-service-data-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | AD Length | 1 | Length of the next AD structure.
uint 8 | AD Type | 1 | 0x01: flags.
uint 8 | Flags | 1 |
uint 8 | AD Length | 1 | Length of the next AD structure.
uint 8 | AD Type | 1 | 0x16: service data with 16 bit service UUID.
uint 16 | Service UUID | 2 | Service UUID: 0xC001, 0xC002, or 0xC003. The last two are deprecated, see service data doc.
[Service data](SERVICE_DATA.md) | Service data | length-3 | Service data, contains state of the Crownstone.
uint 8 | AD Length | 1 | Length of the next AD structure.
uint 8 | AD Type | 1 | 0x08: shortened local name.
char [] | Name | length-1 | The shortened name of this device.



<a name="broadcasts"></a>
# Broadcast commands

Some commands can also be sent via broadcasts. This is the prefered way, as there is no need to connect to the Crownstone, which takes quite some time.
The broadcast protocol is documented in the [broadcast protocol](BROADCAST_PROTOCOL.md) document.



<a name="services"></a>
# Services
When connected, the following services are available.

The AMB columns indicate which users can use these characteristics if encryption is enabled. The access can be further restricted per packet.

- A: Admin
- M: Member
- B: Basic

The following services are available (depending on state and config):
- [Crownstone service](#crownstone_service). Contains all you need: control, config and state.
- [Setup service](#setup_service). Similar to the crownstone service, replaces it when in setup mode.



<a name="crownstone_service"></a>
## Crownstone service

The crownstone service has UUID 24f00000-7d10-4805-bfc1-7663a01c3bff and provides all the functionality of the Crownstone through the following characteristics:

Characteristic | UUID | Date type | Description | A | M | B
--- | --- | --- | --- | :---: | :---: | :---:
Session nonce  | 24f0000e-7d10-4805-bfc1-7663a01c3bff | [Session data](#session_data) | Read the session data. |  |  | ECB
Control        | 24f0000c-7d10-4805-bfc1-7663a01c3bff | [Control packet](#control_packet) | Write a command to the crownstone. | x | x | x
Result         | 24f0000d-7d10-4805-bfc1-7663a01c3bff | [Result packet](#result_packet) | Read the result of a command from the crownstone. | x | x | x
Recovery       | 24f00009-7d10-4805-bfc1-7663a01c3bff | uint32 | Used for [recovery](#recovery). |

Every command written to the control characteristic returns a [result packet](#result_packet) on the result characteristic.
If commands have to be executed sequentially, make sure that the result packet of the previous command was received before calling the next (either by polling or subscribing).

<a name="recovery"></a>
#### Recovery
If you lost your encryption keys you can use this characteristic to factory reset the Crownstone.
This method is only available for 60 seconds after the Crownstone powers on.
You need to write **0xDEADBEEF** to it. Then, the Crownstone disconnects and goes into low TX mode so you'll have to be close to continue the factory reset. After this, you reconnect and write **0xDEADBEEF** again to this characteristic to factory reset the Crownstone.



<a name="setup_service"></a>
## Setup service

The setup service has UUID 24f10000-7d10-4805-bfc1-7663a01c3bff and is only available after a factory reset or when you first power on the Crownstone.
 When encryption is enabled, the control and both config characteristics are encrypted with AES CTR. The key and session nonce for this are gotten from their
 characteristics.

Characteristic | UUID | Date type | Description
--- | --- | --- | ---
MAC address    | 24f10002-7d10-4805-bfc1-7663a01c3bff | uint 8 [6] | Read the MAC address of the crownstone.
Session key    | 24f10003-7d10-4805-bfc1-7663a01c3bff | uint 8 [16] | Read the session key that will be for encryption.
Session data   | 24f1000e-7d10-4805-bfc1-7663a01c3bff | [Session data](#session_data) | Read the session data.
Control        | 24f1000c-7d10-4805-bfc1-7663a01c3bff | [Control packet](#control_packet) | Write a command to the crownstone.
Result         | 24f1000d-7d10-4805-bfc1-7663a01c3bff | [Result packet](#result_packet) | Read the result of a command from the crownstone.

Every command written to the control characteristic returns a [result packet](#result_packet) on the result characteristic.
If commands have to be executed sequentially, make sure that the result packet of the previous command was received before calling the next (either by polling or subscribing).




<a name="data_structs"></a>
# Data structures

Index:

- [Control](#control_packet). Used to send commands to the Crownstone.
- [Result](#result_packet). The result of a command.
- [State](#state_types). State variables of the Crownstone.


<a name="control_packet"></a>
## Control packet

__If encryption is enabled, this packet must be encrypted using any of the keys where the box is checked.__

![Control packet](../docs/diagrams/control-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Protocol | 1 | Which protocol the command is. Should be similar to the protocol as received in the [session data](#session_data). Older protocols might be supported, but there's no guarantee.
uint 16 | [Command type](#command_types) | 2 | Type of the command.
uint 16 | Size | 2 | Size of the payload in bytes.
uint 8 | Payload | Size | Payload data, depends on command type.


<a name="command_types"></a>
## Command types

The AMBS columns indicate which users have access to these commands if encryption is enabled.
Admin access means the packet is encrypted with the admin key.
Setup access means the packet is available in setup mode, and encrypted with the temporary setup key, see [setup](#setup).
- A: Admin
- M: Member
- B: Basic
- S: Setup

Available command types:

Type nr | Type name | Payload type | Result type | Description | A | M | B | S
--- | --- | --- | --- | :---: | :---: | :---: | :---: | :--:
0 | Setup | [Setup packet](#setup_packet) | - | Perform setup. |  |  |  | x
1 | Factory reset | uint 32 | - | Reset device to factory setting, needs Code 0xDEADBEEF as payload | x
2 | Get state | [State get packet](#state_get_packet) | [State get result packet](#state_get_result_packet) | Required access depends on the state type. | x | x | x
3 | Set state | [State set packet](#state_set_packet) | [State set result packet](#state_set_result_packet) | Required access depends on the state type. | x | x | x
4 | Get bootloader version | - | [Bootloader info packet](IPC.md#bootloader-info-packet) | Get bootloader version info. | x | x | x | x
5 | Get UICR data | - | [UICR data packet](#uicr_data_packet) | Get the UICR data. | x | x | x | x
6 | Set ibeacon config ID | [Ibeacon config ID packet](#ibeacon_config_id_packet) | - | Set the ibeacon config ID that is used. The config values can be set via the *Set state* command, with corresponding state ID. You can use this command to interleave between config ID 0 and 1. | x
10 | Reset | - | - | Reset device | x
11 | Goto DFU | - | - | Reset device to DFU mode | x
12 | No operation | - | - | Does nothing, merely there to keep the crownstone from disconnecting | x | x | x
13 | Disconnect | - | - | Causes the crownstone to disconnect | x | x | x
20 | Switch | uint 8 | - | Switch power, 0 = off, 100 = full on | x | x | x | x |
21 | Multi switch | [Multi switch packet](#multi_switch_packet) | - | Switch multiple Crownstones (via mesh). | x | x | x
22 | Dimmer | uint 8 | - | Set dimmer to value, 0 = off, 100 = full on | x | x | x |
23 | Relay | uint 8 | - | Switch relay, 0 = off, 1 = on | x | x | x
30 | Set time | uint 32 | - | Sets the time. Timestamp is in seconds since epoch (Unix time). | x | x |
31 | Increase TX | - | - | Temporarily increase the TX power when in setup mode |  |  |  | x
32 | Reset errors | [Error bitmask](#state_error_bitmask) | - | Reset all errors which are set in the written bitmask. | x
33 | Mesh command | [Command mesh packet](#command_mesh_packet) | - | Send a generic command over the mesh. Required access depends on the command. | x | x | x
34 | Set sun times | [Sun time packet](#sun_time_packet) | - | Update the reference times for sunrise and sunset | x | x
40 | Allow dimming | uint 8 | - | Allow/disallow dimming, 0 = disallow, 1 = allow. | x
41 | Lock switch | uint 8 | - | Lock/unlock switch, 0 = unlock, 1 = lock. | x
50 | UART message | payload | - | Print the payload to UART. | x
60 | Add behaviour | [Add behaviour packet](BEHAVIOUR.md#add_behaviour_packet) | [Index and master hash](BEHAVIOUR.md#add_behaviour_result_packet) | Add a behaviour to an unoccupied index. | x | x
61 | Replace behaviour | [Replace behaviour packet](BEHAVIOUR.md#replace_behaviour_packet) | [Index and master hash](BEHAVIOUR.md#replace_behaviour_result_packet) | Replace the behaviour at given index. | x | x
62 | Remove behaviour | [Remove behaviour packet](BEHAVIOUR.md#remove_behaviour_packet) | [Index and master hash](BEHAVIOUR.md#remove_behaviour_result_packet) | Remove the behaviour at given index. | x | x
63 | Get behaviour | [Index](BEHAVIOUR.md#get_behaviour_packet) | [Index and behaviour packet](BEHAVIOUR.md#get_behaviour_result_packet) | Obtain the behaviour stored at given index. | x | x
64 | Get behaviour indices | - | [Behaviour indices packet](BEHAVIOUR.md#get_behaviour_indices_packet) | Obtain a list of occupied indices in the list of behaviours. | x | x
69 | Get behaviour debug | - | [Behaviour debug packet](#behaviour_debug_packet) | Obtain debug info of the current behaviour state. | x
70 | Register tracked device | [Register tracked device packet](#register_tracked_device_packet) | - | Register or update a device to be tracked.


<a name="setup_packet"></a>
#### Setup packet

![Setup packet](../docs/diagrams/setup-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Stone ID | 1 | Crownstone ID. Should be unique per sphere.
uint 8 | Sphere ID | 1 | Short sphere ID. Should be the same for each Crownstone in the sphere.
uint 8[] | Admin key  | 16 | 16 byte key used to encrypt/decrypt admin access functions.
uint 8[] | Member key | 16 | 16 byte key used to encrypt/decrypt member access functions.
uint 8[] | Basic key  | 16 | 16 byte key used to encrypt/decrypt basic access functions.
uint 8[] | Service data key  | 16 | 16 byte key used to encrypt/decrypt service data.
uint 8[] | Localization key  | 16 | 16 byte key used to encrypt/decrypt localization messages.
uint 8[] | Mesh device key  | 16 | 16 byte key used to encrypt/decrypt mesh config. Should be different for each Crownstone.
uint 8[] | Mesh app key  | 16 | 16 byte key used to encrypt/decrypt mesh messages. Should be the same for each Crownstone in the sphere.
uint 8[] | Mesh net key  | 16 | 16 byte key used to encrypt/decrypt relays of mesh messages. Should be the same for each Crownstone in the sphere.
uint 8[] | iBeacon UUID | 16 | The iBeacon UUID. Should be the same for each Crownstone in the sphere.
uint 16 | iBeacon major | 2 | The iBeacon major. Together with the minor, should be unique per sphere.
uint 16 | iBeacon minor | 2 | The iBeacon minor. Together with the major, should be unique per sphere.


<a name="state_get_packet"></a>
#### State get packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | [State type](#state_types) | 2 | Type of state to get.
uint 16 | id | 2 | ID of state to get. Most state types will only have ID 0.
uint 8 | [Persistence mode](#state_get_persistence_mode) | 1 | Type of persistence mode.
uint 8 | reserved | 1 | Reserved for future use, must be 0 for now.

<a name="state_set_packet"></a>
#### State set packet

Most configuration changes will only be applied after a reboot.

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | [State type](#state_types) | 2 | Type of state to set.
uint 16 | id | 2 | ID of state to get. Most state types will only have ID 0.
uint 8 | [Persistence mode](#state_set_persistence_mode_set) | 1 | Type of persistence mode.
uint 8 | reserved | 1 | Reserved for future use, must be 0 for now.
uint 8 | Payload | N | Payload data, depends on state type.

Most configuration changes will only be applied after a reboot.
Available configurations types:

<a name="state_get_result_packet"></a>
#### State get result packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | [State type](#state_types) | 2 | Type of state.
uint 16 | id | 2 | ID of state.
uint 8 | [Persistence mode](#state_get_persistence_mode) | 1 | Type of persistence mode.
uint 8 | reserved | 1 | Reserved for future use, must be 0 for now.
uint 8 | Payload | N | Payload data, depends on state type.

<a name="state_set_result_packet"></a>
#### State set result packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | [State type](#state_types) | 2 | Type of state.
uint 16 | id | 2 | ID of state that was set.
uint 8 | [Persistence mode](#state_set_persistence_mode_set) | 1 | Type of persistence mode.
uint 8 | reserved | 1 | Reserved for future use, must be 0 for now.

<a name="state_get_persistence_mode"></a>
#### State get persistence mode
Value | Name | Description
--- | --- | ---
0   | CURRENT | Get value from ram if exists, else from flash if exists, else get default. This is the value used by the firmware.
1   | STORED | Get value from flash, else get default. This value will be used after a reboot.
2   | FIRMWARE_DEFAULT | Get default value.

<a name="state_set_persistence_mode_set"></a>
#### State set persistence mode
Value | Name | Description
--- | --- | ---
0   | TEMPORARY | Set value to ram. This value will be used by the firmware, but lost after a reboot.
1   | STORED | Set value to ram and flash. This value will be used by the firmware, also after a reboot. Overwrites the temporary value.


<a name="uicr_data_packet"></a>
##### UICR data packet

This packet is meant for developers. For more information, see [UICR](UICR.md) and [Naming](NAMING.md).

![UICR data packet](../docs/diagrams/uicr_data_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 32 | Board | 4 | The board version.
uint 8 | Product type | 1 | Type of product.
uint 8 | Region | 1 | Which region the product is for.
uint 8 | Product family | 1 | Product family.
uint 8 | Reserved | 1 | Reserved for future use, will be 0xFF for now.
uint 8 | Hardware patch | 1 | Hardware version patch.
uint 8 | Hardware minor | 1 | Hardware version minor.
uint 8 | Hardware major | 1 | Hardware version major.
uint 8 | Reserved | 1 | Reserved for future use, will be 0xFF for now.
uint 8 | Product housing | 1 |
uint 8 | Production week | 1 | Week number.
uint 8 | Production year | 1 | Last 2 digits of the year.
uint 8 | Reserved | 1 | Reserved for future use, will be 0xFF for now.


<a name="ibeacon_config_id_packet"></a>
##### Ibeacon config ID packet

![Ibeacon config ID packet](../docs/diagrams/ibeacon_config_id_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | ID | 1 | The ibeacon config ID to set.
uint 32 | Timestamp | 4 | Unix timestamp when the ibeacon config ID should be set (the first time).
uint 16 | Interval | 2 | Interval in seconds when the ibeacon config ID should be set again, after the given timestamp.

- ID can only be 0 or 1.
- Set the interval to 0 if you want to set the ibeacon config ID only once, at the given timestamp. Use timestamp 0 if you want it to be set immediately.
- To interleave between two config IDs every 60 seconds, you will need two commands. First set ID=0, at timestamp=0 with interval=60, then set ID=1 at timestamp=30 with interval=60.

<a name="sun_time_packet"></a>
##### Sun time packet

![Sun time packet](../docs/diagrams/set_sun_time_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 32 | Sunrise | 4 | The moment when the upper limb of the sun appears on the horizon. Units: seconds since midnight.
uint 32 | Sunset | 4 | The moment when the upper limb of the Sun disappears below the horizon. Units: seconds since midnight.


<a name="multi_switch_packet"></a>
##### Multi switch packet

![Multi switch list](../docs/diagrams/multi_switch_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Count | 1 | Number of valid entries.
[Multi switch entry](#multi_switch_entry_packet) [] | List | Count * 2 | A list of switch commands.

<a name="multi_switch_entry_packet"></a>
##### Multi switch entry

![Multi switch short entry](../docs/diagrams/multi_switch_entry_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Crownstone ID | 1 | The identifier of the crownstone to which this item is targeted.
uint 8 | Switch value | 1 | The switch value to be set by the targeted crownstone. 0 = off, 100 = fully on.


<a name="state_error_bitmask"></a>
#### Error Bitmask

Bit | Name |  Description
--- | --- | ---
0 | Overcurrent | If this is 1, overcurrent was detected.
1 | Overcurrent dimmer | If this is 1, overcurrent for the dimmer was detected.
2 | Chip temperature | If this is 1, the chip temperature is too high.
3 | Dimmer temperature | If this is 1, the dimmer temperature is too high.
4 | Dimmer on failure | If this is 1, the dimmer is broken, in an always (partial) on state.
5 | Dimmer off failure | If this is 1, the dimmer is broken, in an always (partial) off state.
6-31 | Reserved | Reserved for future use.


<a name="command_mesh_packet"></a>
#### Mesh command packet
For now, only a few of commands are implemented:

- Set time, only broadcast, without acks.
- Noop, only broadcast, without acks.
- State set, only 1 target ID, with ack.
- Set ibeacon config ID.

![Command packet](../docs/diagrams/command-mesh-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | [Type](#mesh_command_types) | 1 | Type of command, see table below.
uint 8 | [Flags](#mesh_command_flags) | 1 | Options.
uint 8 | Timeout / transmissions | 1 | When acked: timeout time in seconds. Else: number of times to send the command. 0 to use the default.
uint 8 | ID count | 1 | The number of stone IDs provided.
uint8 [] | List of stone IDs | Count | IDs of the stones at which this message is aimed. Can be empty, then the command payload follows directly after the count field.
uint 8 | Command payload | N | The command payload data, which depends on the [type](#mesh_command_types).

<a name="mesh_command_types"></a>
##### Mesh command types

Type nr | Type name | Payload type | Payload description
--- | --- | --- | ---
0 | Control | [Control](#control_packet) | Send a control command over the mesh, see control packet.

<a name="mesh_command_flags"></a>
##### Mesh command flags

For now there are only a couple of combinations possible:

- If you want to send a command to all stones in the mesh, without acks and retries, set: `Broadcast=true`, `AckIDs=false`, `KnownIDs=false`.
- If you want to send a command to all stones in the mesh, with acks and retries, set: `Broadcast=true`, `AckIDs=true`, `KnownIDs=false`. You will have to provide the list of IDs yourself.
- If you want to send a command to 1 stone, with acks and retries, set: `Broadcast=false`, `AckIDs=true`, `KnownIDs=false`.

Bit | Name |  Description
--- | --- | ---
0 | Broadcast | Send command to all stones. Else, its only sent to all stones in the list of stone IDs, which will take more time.
1 | Ack all IDs | Retry until an ack is received from all stones in the list of stone IDs, or until timeout. **More than 1 IDs without broadcast is not implemented yet.**
2 | Use known IDs | Instead of using the provided stone IDs, use the stone IDs that this stone has seen. **Not implemented yet.**


<a name="behaviour_debug_packet"></a>
##### Behaviour debug packet

![Behaviour debug packet](../docs/diagrams/behaviour_debug_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 32 | Time | 4 | Current time. 0 if not set.
uint 32 | Sunrise | 4 | Sunrise time, seconds after midnight. 0 if not set.
uint 32 | Sunset | 4 | Sunset time, seconds after midnight. 0 if not set.
uint 8 | Override state | 1 | Override state. 254 if not set.
uint 8 | Behaviour state | 1 | Behaviour state. 254 if not set.
uint 8 | Aggregated state | 1 | Aggregated state. 254 if not set.
uint 8 | Dimmer powered | 1 | Whether the dimmer is powered.
uint 8 | Behaviour enabled | 1 | Whether behaviour is enabled.
uint 64 | Stored behaviours | 8 | Bitmask of behaviours that are stored. Nth bit is Nth behaviour index.
uint 64 | Active behaviours | 8 | Bitmask of behaviours that are currently active. Nth bit is Nth behaviour index.
uint 64 | Active end conditions | 8 | Bitmask of behaviours with active end conditions. Nth bit is Nth behaviour index.
uint 64 | Active timeout periods | 8 | Bitmask of behaviours that are in (presence) timeout period. Nth bit is Nth behaviour index.
uint 64[] | Presence | 64 | Bitmask per profile (there are 8 profiles) of occupied rooms. Nth bit is Nth room.

<a name="register_tracked_device_packet"></a>
##### Register tracked device packet

![Register tracked device packet](../docs/diagrams/register_tracked_device_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | Device ID | 2 | Unique ID of the device.
uint 8 | Location ID | 1 | ID of the location where the device is.
uint 8 | Profile ID | 1 | Profile ID of the device.
int 8 | RSSI offset | 1 | Offset from standard signal strength.
uint 8 | Flags | 1 | [Flags](BROADCAST_PROTOCOL.md#background_adv_flags).
uint 24 | Device token | 3 | Token that will be advertised by the device.
uint 16 | Time to live | 2 | Time in minutes after which the device token will be invalid.

<a name="result_packet"></a>
## Result packet

__If encryption is enabled, this packet will be encrypted using any of the keys where the box is checked.__

![Result packet](../docs/diagrams/result-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Protocol | 1 | Which protocol the result is. Should be similar to the protocol in the [control packet](#control_packet).
uint 16 | [Command type](#command_types) | 2 | Type of the command of which this packet is the result.
uint 16 | [Result code](#result_codes) | 2 | The result code.
uint 16 | Size | 2 | Size of the payload in bytes.
uint 8 | Payload | Size | Payload data, depends on command type.

<a name="result_codes"></a>
#### Result codes

Value | Name | Description
--- | --- | ---
0   | SUCCESS | Completed successfully.
1   | WAIT_FOR_SUCCESS | Command is successful so far, but you need to wait for SUCCESS.
2   | ERR_SUCCESS_NO_CHANGE | Command is succesful, but nothing changed.
16  | BUFFER_UNASSIGNED | No buffer was assigned for the command.
17  | BUFFER_LOCKED | Buffer is locked, failed queue command.
18  | BUFFER_TOO_SMALL | Buffer is too small for operation.
32  | WRONG_PAYLOAD_LENGTH | Wrong payload lenght provided.
33  | WRONG_PARAMETER | Wrong parameter provided.
34  | INVALID_MESSAGE | invalid message provided.
35  | UNKNOWN_OP_CODE | Unknown operation code provided.
36  | UNKNOWN_TYPE | Unknown type provided.
37  | NOT_FOUND | The thing you were looking for was not found.
38  | NO_SPACE | There is no space for this command.
39  | BUSY | Wait for something to be done.
40  | ERR_WRONG_STATE | The crownstone is in a wrong state.
41  | ERR_ALREADY_EXISTS | Item already exists.
42  | ERR_TIMEOUT | Operation timed out.
43  | ERR_CANCELED | Operation was canceled.
44  | ERR_PROTOCOL_UNSUPPORTED | The protocol is not supported.
48  | NO_ACCESS | Invalid access for this command.
49  | ERR_UNSAFE | It's unsafe to execute this command.
64  | NOT_AVAILABLE | Command currently not available.
65  | NOT_IMPLEMENTED | Command not implemented (not yet or not anymore).
67  | NOT_INITIALIZED | Something must first be initialized.
68  | ERR_NOT_STARTED | Something must first be started.
80  | WRITE_DISABLED | Write is disabled for given type.
81  | ERR_WRITE_NOT_ALLOWED | Direct write is not allowed for this type, use command instead.
96  | ADC_INVALID_CHANNEL | Invalid adc input channel selected.
112 | ERR_EVENT_UNHANDLED | The event or command was not handled.
65535 | UNSPECIFIED | Unspecified error.



<a name="state_types"></a>
## State types

The AMBS columns indicate which users have access to these states: `r` for read access, `w` for write access.
Admin access means the packet is encrypted with the admin key.
Setup access means the packet is available in setup mode, and encrypted with the temporary setup key, see [setup](#setup).
- A: Admin
- M: Member
- B: Basic
- S: Setup

Type nr | Type name | Payload type | Description | A | M | B
------- | ---------- | ------------- | ------------ | --- | --- | ---
5 | PWM period | uint 32 | Sets PWM period in μs for the dimmer. **Setting this to a wrong value may cause damage.**  | rw |  | 
6 | iBeacon major | uint 16 | iBeacon major number.  | rw |  | 
7 | iBeacon minor | uint 16 | iBeacon minor number.  | rw |  | 
8 | iBeacon UUID | uint 8 [16] | iBeacon UUID.  | rw |  | 
9 | iBeacon TX power | int 8 | iBeacon signal strength at 1 meter.  | rw |  | 
11 | TX power | int 8 | TX power, can be: -40, -20, -16, -12, -8, -4, 0, or 4.  | rw |  | 
12 | Advertisement interval | uint 16 | Advertisement interval between 0x0020 and 0x4000 in units of 0.625 ms.  | rw |  | 
16 | Scan duration | uint 16 | Scan duration in ms. Only used by interval scanner, which isn't used by default. **Deprecated**  | rw |  | 
18 | Scan break duration | uint 16 | Waiting time in ms to start next scan. Only used by interval scanner, which isn't used by default. **Deprecated**  | rw |  | 
19 | Boot delay | uint 16 | Time to wait with radio after boot (ms). **Setting this to a wrong value may cause damage.**  | rw |  | 
20 | Max chip temp | int 8 | If the chip temperature (in degrees Celcius) goes above this value, the power gets switched off. **Setting this to a wrong value may cause damage.**  | rw |  | 
24 | Mesh enabled | uint 8 | Whether mesh is enabled.  | rw |  | 
25 | Encryption enabled | uint 8 | Whether encryption is enabled. **Not implemented** |  |  | 
26 | iBeacon enabled | uint 8 | Whether iBeacon is enabled. **Not implemented** |  |  | 
27 | Scanner enabled | uint 8 | Whether device scanning is enabled. | rw |  | 
33 | Sphere id | uint 8 | Short id of the sphere this Crownstone is part of. | rw |  | 
34 | Crownstone id | uint 8 | Crownstone identifier used in advertisement package. | rw |  | 
35 | Admin key | uint 8 [16] | 16 byte key used to encrypt/decrypt owner access functions. |  |  | 
36 | Member key | uint 8 [16] | 16 byte key used to encrypt/decrypt member access functions. |  |  | 
37 | Basic key | uint 8 [16] | 16 byte key used to encrypt/decrypt basic access functions. |  |  | 
39 | Scan interval | uint 16 | The scan interval in units of 0.625 millisecond. Only used by interval scanner, which isn't used by default. | rw |  | 
40 | Scan window | uint 16 | The scan window to in units of 0.625 millisecond. Only used by interval scanner, which isn't used by default. | rw |  | 
41 | Relay high duration | uint 16 | The time/duration that the relay is powered for a switch (ms). **Setting this to a wrong value may cause damage.** | rw |  | 
42 | Low TX power | int 8 | The TX power used when in low transmission power for bonding (can be: -40, -20, -16, -12, -8, -4, 0, or 4). | rw |  | 
43 | Voltage multiplier | float | Voltage multiplier (for power measurement). **Setting this to a wrong value may cause damage.** | rw |  | 
44 | Current multiplier | float | Current multiplier (for power measurement). **Setting this to a wrong value may cause damage.** | rw |  | 
45 | Voltage zero | int 32 | Voltage zero level (for power measurement).      **Setting this to a wrong value may cause damage.** | rw |  | 
46 | Current zero | int 32 | Current zero level (for power measurement).      **Setting this to a wrong value may cause damage.** | rw |  | 
47 | Power zero | int 32 | Power zero level in mW (for power measurement).    **Setting this to a wrong value may cause damage.** | rw |  | 
50 | Current consumption threshold | uint 16 | At how much mA the switch will be turned off (soft fuse).            **Setting this to a wrong value may cause damage.** | rw |  | 
51 | Current consumption threshold dimmer | uint 16 | At how much mA the dimmer will be turned off (soft fuse).     **Setting this to a wrong value may cause damage.** | rw |  | 
52 | Dimmer temp up voltage | float | Voltage of upper threshold of the dimmer thermometer.                         **Setting this to a wrong value may cause damage.** | rw |  | 
53 | Dimmer temp down voltage | float | Voltage of lower threshold of the dimmer thermometer.                       **Setting this to a wrong value may cause damage.** | rw |  | 
54 | Dimming allowed | uint8 | Whether this Crownstone is allowed to dim. | rw |  | 
55 | Switch locked | uint8 | Whether this Crownstone is allowed to change the switch state. | rw |  | 
56 | Switchcraft enabled | uint8 | Whether this Crownstone has switchcraft enabled. | rw |  | 
57 | Switchcraft threshold | float | Sets the threshold for switchcraft. A higher threshold will generally make it less likely to detect a switch (less true and false positives). **Setting this to a wrong value may cause damage.** | rw |  | 
59 | UART enabled | uint 8 | Whether UART is enabled, 0 = none, 1 = RX only, 3 = TX and RX. | rw |  | 
60 | Device name | char [] | Name of the device. | rw |  | 
61 | Service data key | uint 8 [16] | 16 byte key used to encrypt/decrypt service data. |  |  | 
62 | Mesh device key | uint 8 [16] | 16 byte key used to encrypt/decrypt mesh messages to configure this Crownstone. |  |  | 
63 | Mesh application key | uint 8 [16] | 16 byte key used to encrypt/decrypt mesh messages for the application of this Crownstone. |  |  | 
64 | Mesh network key | uint 8 [16] | 16 byte key used to encrypt/decrypt mesh messages to be received or relayed by this Crownstone. |  |  | 
65 | Localization key | uint 8 [16] | 16 byte key used to encrypt/decrypt messages to tell your location to this Crownstone. |  |  | 
66 | Start dimmer on zero crossing | uint 8 | Whether the dimmer should start on a zero crossing or not. | rw |  | 
67 | Tap to toggle enabled | uint 8 | Whether tap to toggle is enabled on this Crownstone. | rw |  | 
68 | Tap to toggle RSSI threshold offset | int 8 | RSSI threshold offset from default, above which tap to toggle will respond. | rw |  | 
128 | Reset counter | uint 16 | Counts the number of resets. | r | r | 
129 | [Switch state](#switch_state_packet) | uint 8 | Current switch state. | r | r | 
130 | Accumulated energy | int 64 | Accumulated energy in μJ. | r | r | 
131 | Power usage | int 32 | Current power usage in mW. | r | r | 
134 | Operation Mode | uint 8 | Internal usage. |  |  | 
135 | Temperature | int 8 | Chip temperature in °C. | r | r | 
136 | Time | uint 32 | The current time as unix timestamp. | r | r | 
139 | [Error bitmask](#state_error_bitmask) | uint 32 | Bitmask with errors. | r | r | 
149 | Sun time | [Sun time packet](#sun_time_packet) | Packet with sun rise and set times. | r | r | 
150 | Behaviour settings | [Behaviour settings](#behaviour_settings_packet) | Behaviour settings. | rw | rw | r

<a name="switch_state_packet"></a>
#### Switch state
To be able to distinguish between the relay and dimmer state, the switch state is a bit struct with the following layout:

![Switch state packet](../docs/diagrams/switch_state_packet.png)

Bit | Name |  Description
--- | --- | ---
0 | Relay | Value of the relay, where 0 = OFF, 1 = ON.
1-7 | Dimmer | Value of the dimmer, where 100 if fully on, 0 is OFF, dimmed in between.

<a name="behaviour_settings_packet"></a>
##### Behaviour settings

![Behaviour settings packet](../docs/diagrams/behaviour_settings_packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 32 | [Flags](#behaviour_settings_flags) | 4 | Flags.


<a name="behaviour_settings_flags"></a>
##### Behaviour settings flags

![Behaviour settings flags](../docs/diagrams/behaviour_settings_flags.png)

Bit | Name |  Description
--- | --- | ---
0 | Enabled | Whether behaviours are enabled.
1-31 | Reserved | Reserved for future use, should be 0 for now.
