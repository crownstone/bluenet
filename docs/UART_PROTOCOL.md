# UART Protocol
---------------

## Special characters

There are two special characters:
- Start: `0x7E`
- Escape: `0x5C`

Every byte in the UART message that equals one of the special characters will be XORed by `0x40`, and prepended by the escape character.

## Wrapper

Every UART message is wrapped.

Type | Name | Length | Description
--- | --- | --- | ---
uint8   | Start   | 1 | Start of a message.
uint16  | OpCode  | 2 | Type of message.
uint16  | Size    | 2 | Size of the message in bytes.
uint8[] | Message | N | The message.
uint16  | CRC     | 2 | The CRC16 of OpCode, Size, and message.

## RX OpCodes

Opcodes for messages received by the Crownstone.

Type  | Packet | Description
----- | ------ | ----
1     | [Control msg](../docs/PROTOCOL.md#control_packet).
10000 | uint8  | Enable/disable advertising.
10001 | uint8  | Enable/disable mesh.
10002 | -      | Get ID of this Crownstone.
10003 | -      | Get MAC address of this Crownstone.
10103 | -      | Increase the range on the current channel.
10104 | -      | Decrease the range on the current channel.
10105 | -      | Increase the range on the voltage channel.
10106 | -      | Decrease the range on the voltage channel.
10108 | uint8  | Enable/disable differential mode on current channel. (Currently the packet is ignored, and it toggles instead)
10109 | uint8  | Enable/disable differential mode on voltage channel. (Currently the packet is ignored, and it toggles instead)
10110 | uint8  | Change the pin used on voltage channel. (Currently the packet is ignored, and it rotates between certain pins instead)
10200 | uint8  | Enable sending current samples.
10201 | uint8  | Enable sending voltage samples.
10202 | uint8  | Enable sending filtered current samples.
10204 | uint8  | Enable sending calculated power samples.

## TX OpCodes

Opcodes for messages sent by the Crownstone.

Type  | Packet | Description
----- | ------ | ----
0     | ?      | Ack. (Not implemented yet)
2     | [Service data](../docs/SERVICE_DATA.md) | Service data of this Crownstone (unencrypted).
3     | string | As requested via control command `UART message`.
100   | [Mesh state](../docs/PROTOCOL.md#mesh-state-packet) | State of other Crownstones in the mesh (channel 0). **Deprecated**
101   | [Mesh state](../docs/PROTOCOL.md#mesh-state-packet) | State of other Crownstones in the mesh (channel 1). **Deprecated**
102   | [Service data](../docs/SERVICE_DATA.md#service_data_encrypted) | State of other Crownstones in the mesh (unencrypted). **Not implemented yet**

10000 | uint8  | Whether advertising is enabled.
10001 | uint8  | Whether mesh is enabled.
10002 | uint8  | Own Crownstone ID.
10003 | MAC    | Own mac address (6 bytes).
10100 | [ADC config](#adc_channel_config_packet) | ADC configuration.
10200 | [Current samples](#current_samples_packet) | Raw ADC samples of the current channel.
10201 | [Voltage samples](#voltage_samples_packet) | Raw ADC samples of the voltage channel.
10202 | [Filtered current samples](#current_samples_packet) | Filtered ADC samples of the current channel.
10203 | [Filtered voltage samples](#voltage_samples_packet) | Filtered ADC samples of the voltage channel.
10204 | [Power calculations](#power_calculation_packet) | Calculated power values.
20000 | string | Debug strings.

## Packets

<a name="adc_config_packet"></a>
### ADC config

Sent when the ADC config changes.

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Count | 1 | Number of channels.
[channel_config](#adc_channel_config_packet)[] | Channels |  | List of channel configs.
uint32 | Sampling period | 4 | Sampling period in μs. Each period, all channels are sampled once.


<a name="adc_channel_config_packet"></a>
### ADC channel config

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Pin | 1 | Analog pin number (AIN..). 100 for Vdd.
uint32 | Range | 4 | Range in mV. Max is 3600.
uint8 | RefPin | 1 | Reference pin for differential measurements. Set to 255 to disable differential measurements.


<a name="current_samples_packet"></a>
### Current samples

Type | Name | Length | Description
--- | --- | --- | ---
uint32  | Timestamp | 4 | Counter of the RTC (running at 32768 Hz, max value is 0x00FFFFFF).
int16[] | Samples | 200 | Raw sample data.

<a name="voltage_samples_packet"></a>
### Voltage samples

Type | Name | Length | Description
--- | --- | --- | ---
uint32  | Timestamp | 4 | Counter of the RTC (running at 32768 Hz, max value is 0x00FFFFFF).
int16[] | Samples | 200 | Raw sample data.

<a name="power_calculation_packet"></a>
### Power calculations

Type | Name | Length | Description
--- | --- | --- | ---
uint32 | Timestamp | 4 | Counter of the RTC (running at 32768 Hz, max value is 0x00FFFFFF).
int32  | currentRmsMA | 4 | 
int32  | currentRmsMedianMA | 4 | 
int32  | filteredCurrentRmsMA | 4 | 
int32  | filteredCurrentRmsMedianMA | 4 | 
int32  | avgZeroVoltage | 4 | 
int32  | avgZeroCurrent | 4 | 
int32  | powerMilliWattApparent | 4 | 
int32  | powerMilliWattReal | 4 | 
int32  | avgPowerMilliWattReal | 4 | 



