# Bluenet protocol v4.0.0
-------------------------

This only documents the latest protocol, older versions can be found in the git history.

# Index

- [Setup](#setup). How to setup the crownstone.
- [Encryption](#encryption). How to encrypt and decrypt the data.
- [Advertisements](#advertisement_data). What data is broadcasted by the crownstones.
- [Broadcast commands](#broadcasts). Broadcast commands.
- [Services and characteristics](#services). Which Bluetooth GATT services and characteristics the crownstones have.
- [Data structures](#data_structs). The data structures used for the characteristics, advertisements, and mesh.
    - [Control](#command_types). Used to send commands to the Crownstone.
    - [Result](#result_packet). The result of a command.
    - [State](#state_types). State variables of the Crownstone.


<a name="setup"></a>
# Setup mode
When a Crownstone is new or factory reset, it will go into setup mode.

The setup process goes as follows:

- Crownstone is in setup mode ([Setup service](#setup_service) active).
- Phone connects to the Crownstone.
- Phone reads the **session key** and **session nonce** from the [setup service](#setup_service). These characteristics are not encrypted.
The values are only valid for this connection session. The session key and the session nonce will be used to encrypt the rest of the setup phase using AES 128 CTR as explained [here](#encrypted_write_read).
- Phone subscribes to [control](#setup_service) characteristic.
- Phone commands Crownstone to setup via the control characteristic.
- Phone waits for control characteristic result to become SUCCESS (See [result packet](#result_packet)).
- Crownstone will reboot to normal mode.



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
uint8 [] | Packet nonce | 3 | Packet nonce, sent with every packet (see [encrypted packet](#encrypted_packet)).
uint8 [] | Session nonce | 5 | Session nonce, can be [read](#session_nonce) once after connect (only changes on connect).

<a name="session_nonce"></a>
### Session nonce

After connecting, you first have to read the session nonce from the [Crownstone service](#crownstone_service). The session nonce is [ECB encrypted](#ecb_encryption) with the basic key. After decryption, you should verify whether you have read and decrypted succesfully by checking if the validation in the [data](#encrypted_session_nonce) is equal to **0xCAFEBABE**. If so, you now have the correct session nonce.

The session nonce has two purposes:

- Validation: the first 4 bytes of the session nonce is what we call the **validation key**, it is used in every [encrypted packet](#encrypted_packet), to verify that the correct key was used for decryption/encryption.
- Encryption: the whole 5 bytes are used for the nonce, which is used for CTR encryption.

The session nonce and validation key are only valid during the connection.

<a name="encrypted_session_nonce"></a>
#### Session nonce after ECB decryption

![Encrypted session nonce](../docs/diagrams/encrypted-session-nonce.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 32 | Validation | 4 | 0xCAFEBABE as validation.
uint8 [] | Session nonce | 5 | The session nonce for this session.
uint8 [] | Padding | 7 | Zero-padding so that the whole packet is 16 bytes.


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
### Encrypted Packet

![Encrypted packet](../docs/diagrams/encrypted-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8 [] | Packet nonce | 3 | First 3 bytes of nonce used in the encryption of this message (see [CTR encryption](#ctr_encryption)).
uint8 | User level | 1 | 0: Admin, 1: Member, 2: Basic, 100: Setup
[Encrypted payload](#encrypted_payload) | Encrypted payload | N*16 | The encrypted payload of N blocks.

<a name="encrypted_payload"></a>
#### Encrypted payload

![Encrypted payload](../docs/diagrams/encrypted-payload.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint32 | Validation key | 4 | Should be equal to the read [validation key](#session_nonce).
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
Control        | 24f0000a-7d10-4805-bfc1-7663a01c3bff | [Control packet](#control_packet) | Write a command to the crownstone. | x | x | x
Result         | 24f0000b-7d10-4805-bfc1-7663a01c3bff | [Result packet](#result_packet) | Read the result of a command from the crownstone. | x | x | x
Session nonce  | 24f00008-7d10-4805-bfc1-7663a01c3bff | uint 8 [5] | Read the [session nonce](#session_nonce). First 4 bytes are also used as validation key. |  |  | ECB
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
GoTo DFU       | 24f10006-7d10-4805-bfc1-7663a01c3bff | uint 8 | Write 66 to go to DFU.
Session nonce  | 24f10008-7d10-4805-bfc1-7663a01c3bff | uint 8 [5] | Read the session nonce. First 4 bytes are also used as validation key.
Control        | 24f1000a-7d10-4805-bfc1-7663a01c3bff | [Control packet](#control_packet) | Write a command to the crownstone.
Result         | 24f1000b-7d10-4805-bfc1-7663a01c3bff | [Result packet](#result_packet) | Read the result of a command from the crownstone.

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
uint 16 | [Command type](#command_types) | 2 | Type of the command.
uint 16 | Size | 2 | Size of the payload in bytes.
uint 8 | Payload | Size | Payload data, depends on command type.


<a name="result_code_packet"></a>
### Result code packet

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

Type nr | Type name | Payload type | Description | A | M | B | S
--- | --- | --- | --- | :---: | :---: | :---: | :--:
0 | Setup | [Setup packet](#setup_packet) | Perform setup. |  |  |  | x
1 | Factory reset | uint 32 | Reset device to factory setting, needs Code 0xdeadbeef as payload | x
2 | Get state | [State get packet](#state_get_packet) | Required access depends on the state type. | x | x | x
3 | Set state | [State set packet](#state_set_packet) | Required access depends on the state type. | x | x | x
10 | Reset | - | Reset device | x
11 | Goto DFU | - | Reset device to DFU mode | x
12 | No operation | - | Does nothing, merely there to keep the crownstone from disconnecting | x | x | x
13 | Disconnect | - | Causes the crownstone to disconnect | x | x | x
20 | Switch | uint 8 | Switch power, 0 = off, 100 = full on | x | x | x | x |
21 | Multi switch | [Multi switch packet](#multi_switch_packet) | Switch multiple Crownstones (via mesh). | x | x | x
22 | Dimmer | uint 8 | Set dimmer to value, 0 = off, 100 = full on | x | x | x |
23 | Relay | uint 8 | Switch relay, 0 = off, 1 = on | x | x | x
30 | Set time | uint 32 | Sets the time. Timestamp is in seconds since epoch (Unix time). | x | x |
31 | Increase TX | - | Temporarily increase the TX power when in setup mode |  |  |  | x
32 | Reset errors | [Error bitmask](#state_error_bitmask) | Reset all errors which are set in the written bitmask. | x
33 | Mesh command | [Command mesh packet](#command_mesh_packet) | Send a generic command over the mesh. Required access depends on the command. Required access depends on the command. | x | x | x
40 | Allow dimming | uint 8 | Allow/disallow dimming, 0 = disallow, 1 = allow. | x
41 | Lock switch | uint 8 | Lock/unlock switch, 0 = unlock, 1 = lock. | x
42 | Enable switchcraft | uint 8 | Enable/disable switchcraft. | x
50 | UART message | payload | Print the payload to UART. | x
51 | UART enable | uint 8 | Set UART enabled, 0 = none, 1 = RX only, 3 = TX and RX | x
60 | Save Behaviour | [Save Behaviour packet](BEHAVIOUR.md#save_behaviour_packet) | Save a Behaviour to an unoccupied index | x | x
61 | Replace Behaviour | [Replace Behaviour packet](BEHAVIOUR.md#replace_behaviour_packet) | Replace the Behaviour at given index | x | x
62 | Remove Behaviour | [Remove Behaviour packet](BEHAVIOUR.md#remove_behaviour_packet) | Remove the Behaviour at given index | x | x
63 | Get Behaviour | [Get Behaviour packet](BEHAVIOUR.md#get_behaviour_packet) | Obtain the Behaviour stored at given index | x | x
64 | Get Behaviour Indices | [Get Behaviour Indices packet](BEHAVIOUR.md#get_behaviour_indices_packet) | Obtain a list of occupied indices in the behaviour store | x | x 



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
uint 16  | [State type](#state_types) | 2 | Type of state to get.


<a name="state_set_packet"></a>
#### State set packet

Most configuration changes will only be applied after a reboot.

Type | Name | Length | Description
--- | --- | --- | ---
uint 16  | [State type](#state_types) | 2 | Type of state to set.
uint 8 | Payload | N | Payload data, depends on state type.

Most configuration changes will only be applied after a reboot.
Available configurations types:

<a name="state_result_packet"></a>
#### State result packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 16  | [State type](#state_types) | 2 | Type of state.
uint 8 | Payload | N | Payload data, depends on state type.


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

![Command packet](../docs/diagrams/command-mesh-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | [Type](#mesh_command_types) | 1 | Type of command, see table below.
uint 8 | Reserved | 1 | Reserved for future use, should be 0 for now.
uint 8 | Count | 1 | The number of IDs provided as targets, 0 for broadcast. **Currently, only 0 is implemented.**
uint8 [] | List of target IDs | Count | Crownstone identifiers of the devices at which this message is aimed. For broadcast, no IDs are provided and the command follows directly after the count field.
uint 8 | Command payload | N | The command payload data, which depends on the type.

<a name="mesh_command_types"></a>
##### Mesh command types

Type nr | Type name | Payload type | Payload description
--- | --- | --- | ---
0 | Control | [Control](#control_packet) | Send a control command over the mesh, see control packet. **Currently, only control commands `No operation` and `Set time` are implemented.**



<a name="result_packet"></a>
## Result packet

__If encryption is enabled, this packet will be encrypted using any of the keys where the box is checked.__

![Result packet](../docs/diagrams/result-packet.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | [Command type](#command_types) | 2 | Type of the command of which this packet is the result.
uint 16 | [Result code](#result_codes) | 2 | The result code.
uint 16 | Size | 2 | Size of the payload in bytes.
uint 8 | Payload | Size | Payload data, depends on command type.

<a name="result_codes"></a>
#### Result codes

Value | Name | Description
--- | --- | ---
0 | SUCCESS | Completed successfully.
1 | WAIT_FOR_SUCCESS | Command is successful so far, but you need to wait for SUCCESS.
16 | BUFFER_UNASSIGNED | No buffer was assigned for the command.
17 | BUFFER_LOCKED | Buffer is locked, failed queue command.
18 | BUFFER_TOO_SMALL | Buffer is too small for operation.
32 | WRONG_PAYLOAD_LENGTH | Wrong payload lenght provided.
33 | WRONG_PARAMETER | Wrong parameter provided.
34 | INVALID_MESSAGE | invalid message provided.
35 | UNKNOWN_OP_CODE | Unknown operation code provided.
36 | UNKNOWN_TYPE | Unknown type provided.
37 | NOT_FOUND | The thing you were looking for was not found.
38 | NO_SPACE | There is no space for this command.
39 | BUSY | Wait for something to be done.
48 | NO_ACCESS | Invalid access for this command.
64 | NOT_AVAILABLE | Command currently not available.
65 | NOT_IMPLEMENTED | Command not implemented (not yet or not anymore).
67 | NOT_INITIALIZED | Something must first be initialized.
80 | WRITE_DISABLED | Write is disabled for given type.
81 | ERR_WRITE_NOT_ALLOWED | Direct write is not allowed for this type, use command instead.
96 | ADC_INVALID_CHANNEL | Invalid adc input channel selected.
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
39 | Scan interval | uint 16 | Set the scan interval in units of 0.625 millisecond. Only used by interval scanner, which isn't used by default. | rw |  | 
40 | Scan window | uint 16 | Set the scan window to in units of 0.625 millisecond. Only used by interval scanner, which isn't used by default. | rw |  | 
41 | Relay high duration | uint 16 | Set the time/duration that the relay is powered for a switch (ms). **Setting this to a wrong value may cause damage.** | rw |  | 
42 | Low TX power | int 8 | Set the tx power used when in low transmission power for bonding (can be: -40, -20, -16, -12, -8, -4, 0, or 4). | rw |  | 
43 | Voltage multiplier | float | Set the voltage multiplier (for power measurement). **Setting this to a wrong value may cause damage.** | rw |  | 
44 | Current multiplier | float | Set the current multiplier (for power measurement). **Setting this to a wrong value may cause damage.** | rw |  | 
45 | Voltage zero | int 32 | Set the voltage zero level (for power measurement).      **Setting this to a wrong value may cause damage.** | rw |  | 
46 | Current zero | int 32 | Set the current zero level (for power measurement).      **Setting this to a wrong value may cause damage.** | rw |  | 
47 | Power zero | int 32 | Set the power zero level in mW (for power measurement).    **Setting this to a wrong value may cause damage.** | rw |  | 
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
67 | Tap to toggle RSSI threshold | int 8 | RSSI threshold (after adjustment from the offset) above which tap to toggle will respond. | rw |  | 
128 | Reset counter | uint 16 | Counts the number of resets. | r | r | 
129 | [Switch state](#switch_state_packet) | uint 8 | Current switch state. | r | r | 
130 | Accumulated energy | int 64 | Accumulated energy in μJ. | r | r | 
131 | Power usage | int 32 | Current power usage in mW. | r | r | 
134 | Operation Mode | uint 8 | Internal usage. |  |  | 
135 | Temperature | int 8 | Chip temperature in °C. | r | r | 
136 | Time | uint 32 | The current time as unix timestamp. | r | r | 
139 | [Error bitmask](#state_error_bitmask) | uint 32 | Bitmask with errors. | r | r | 

<a name="switch_state_packet"></a>
#### Switch state
To be able to distinguish between the relay and dimmer state, the switch state is a bit struct with the following layout:

![Switch State Packet](../docs/diagrams/switch_state_packet.png)

Bit | Name |  Description
--- | --- | ---
0 | Relay | Value of the relay, where 0 = OFF, 1 = ON.
1-7 | Dimmer | Value of the dimmer, where 100 if fully on, 0 is OFF, dimmed in between.
