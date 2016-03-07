#### Power curve packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 16 | numSamples     | 2              | Number of current samples + voltage samples, including the first samples.
uint 16 | firstCurrent   | 2              | First current sample.
uint 16 | lastCurrent    | 2              | Last current sample.
uint 16 | firstVoltage   | 2              | First voltage sample.
uint 16 | lastVoltage    | 2              | Last voltage sample.
uint 32 | firstTimeStamp | 4              | Timestamp of first current sample.
uint 32 | lastTimeStamp  | 4              | Timestamp of last sample.
int 8  | currentDiffs   | numSamples/2-1 | Array of differences with previous current sample.
int 8  | voltageDiffs   | numSamples/2-1 | Array of differences with previous voltage sample.
int 8  | timeDiffs      | numSamples-1   | Array of differences with previous timestamp.


#### <a name="config_packet"></a>Configuration packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 8  | type | 1 | Type, see table below.
uint 8  | reserved | 1 | Not used: reserved for alignment.
uint 16 | length | 2 | Length of the payload in bytes.
uint 8 | payload | length | Payload data, depends on type.


Type nr | Type name | Payload type | Payload description
--- | --- | --- | ---
1 or 0x01 | Device name | char array | Name of the device.
2 or 0x02 | Room | uint 8 | Room number.
3 or 0x03 | Floor | uint 8 | Floor number.
4 or 0x04 | Nearby timeout | uint 16 | Time in ms before switching off when noone is nearby (not implemented yet).
5 or 0x05 | PWM frequency | uint 8 | Sets PWM frequency (not implemented yet).
6 or 0x06 | iBeacon major | uint 16 | iBeacon major number.
7 or 0x07 | iBeacon minor | uint 16 | iBeacon minor number.
8 or 0x08 | iBeacon UUID | 16 bytes | iBeacon UUID.
9 or 0x09 | iBeacon RSSI | int 8 | iBeacon RSSI at 1 meter.
10 or 0x0A | wifi settings | char array | Json with the wifi settings: `{ "ssid": "<name here>", "key": "<password here>"}`.
11 or 0x0B | TX power | int 8 | TX power, can be: -40, -30, -20, -16, -12, -8, -4, 0, or 4.
12 or 0x0C | Advertisement interval | uint 16 | Advertisement interval between 0x0020 and 0x4000 in units of 0.625 ms.
13 or 0x0D | passkey | char array | Passkey of the device: must be 6 digits.
14 or 0x0E | min env temp | int 8 | If temperature (in degrees Celcius) goes below this value, send an alert (not implemented yet).
15 or 0x0F | max env temp | int 8 | If temperature (in degrees Celcius) goes above this value, send an alert (not implemented yet).
16 or 0x10 | scan duration | uint 16 | Scan duration in ms. *Setting this too high may cause the device to reset during scanning.*
17 or 0x11 | scan send delay | uint 16 | Time in ms to wait before sending scan results over the mesh. *Setting this too low may cause the device to reset during scanning.*
18 or 0x12 | scan break duration | uint 16 | Waiting time in ms between sending results and next scan. *Setting this too low may cause the device to reset during scanning.*
19 or 0x13 | boot delay | uint 16 | Time to wait with radio after boot, **Setting this too low may cause the device to reset on boot.**
20 or 0x14 | max chip temp | int 8 | If the chip temperature (in degrees Celcius) goes above this value, the power gets switched off.
21 or 0x15 | scan filter | uint 8 | Filter out certain types of devices from the scan results (1 for GuideStones, 2 for CrownStones, 3 for both).
22 or 0x16 | scan filter fraction | uint 16 | If scan filter is set, do *not* filter them out each every X scan results.


#### <a name="scan_result_packet"></a>Scan result packet

Type | Name | Length | Description
--- | --- | --- | ---
byte array | Address | 6 | Bluetooth address of the scanned device.
int 8 | RSSI | 1 | Average RSSI to the scanned device.
uint 16 | Occurrences | 2 | Number of times the devices was scanned.

#### <a name="scan_result_list_packet"></a>Scan result list packet

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | size | 1 | Number of scanned devices in the list.
[Scan result packet](#scan_result_packet) | size * 9 | Array of scan result packets.

#### Mesh payload packet

Type | Name | Length | Description
--- | --- | --- | ---
byte array | Target address | 6 | Bluetooth address of the device at which this message is aimed at, all zero for any device.
uint 16 | Type | 2 | Type of message, see table below.
byte array | Payload | 0 to 91 | Payload data, depends on type.

Type nr | Type name | Payload type | Payload description
--- | --- | --- | ---
0 | Event | uint 16 | Event type that happened.
1 | Power | uint 8 | Current power usage.
2 | Beacon | beacon mesh | Configure the iBeacon settings.
3 | Command | mesh command | Send a command over the mesh.
4 | Config | [Configuration packet](#config_packet) | Send a configuration.
101 | Scan result | [Scan result list packet](#scan_result_list_packet) | List of scanned devices.

