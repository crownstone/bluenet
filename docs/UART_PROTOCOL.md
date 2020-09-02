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
1     | [Control msg](../docs/PROTOCOL.md#control_packet). | The result will be returned.
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
1     | [Result packet](../docs/PROTOCOL.md#result_packet) | Result of a control command.
2     | [Service data with device type](../docs/SERVICE_DATA.md#service_data_header) | Service data of this Crownstone (unencrypted).
3     | string | As requested via control command `UART message`.
4     | [Presence change packet](#presence_change_packet) | Sent when the presence has changed. Note: a profile ID can be at multiple locations at the same time.
102   | [Service data without device type](../docs/SERVICE_DATA.md#service_data_encrypted) | State of other Crownstones in the mesh (unencrypted).
103   | [External state part 0](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_state_0_t) | Part of the state of other Crownstones in the mesh.
104   | [External state part 1](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_state_1_t) | Part of the state of other Crownstones in the mesh.
105   | [Mesh result](#mesh_result_packet) | Result of an acked mesh command. You will get a mesh result for each Crownstone, also when it timed out. Note: you might get this multiple times for the same ID.
106   | [Mesh ack all result](../docs/PROTOCOL.md#result_packet) | SUCCESS when all IDs were acked, or TIMEOUT if any timed out.
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
10403 | [Time](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_time_t) | Received command to set time from the mesh.
10410 | [Profile location](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_profile_location_t) | Received the location of a profile from the mesh.
10411 | [Behaviour settings](../docs/MESH_PROTOCOL.md#behaviour_settings_t) | Received command to set behaviour settings from the mesh.
10412 | [Tracked device register](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_device_register_t) | Received command to register a tracked device from the mesh.
10413 | [Tracked device token](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_device_token_t) | Received command to set the token of a tracked device from the mesh.
10414 | [Sync request](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_sync_request_t) | Received a sync request from the mesh.
10420 | [Tracked device heartbeat](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_device_heartbeat_t) | Received heartbeat command of a tracked device from the mesh.
20000 | string | Debug strings.

## Packets

<a name="presence_change_packet"></a>
### Presence change packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | [Type](#presence_change_type) | 1 | Type of change.
uint8 | Profile ID | 1 | ID of the profile.
uint8 | Location ID | 1 | ID of the location.


<a name="presence_change_type"></a>
##### Presence change type

Value | Name | Description
--- | --- | ---
0 | First sphere enter     | The first user entered the sphere. Ignore profile and location values.
1 | Last sphere exit       | The last user left the sphere. Ignore profile and location values.
2 | Profile sphere enter   | The first user of given profile entered the sphere. Ignore location value.
3 | Profile sphere exit    | The last user of given profile left the sphere. Ignore location value.
4 | Profile location enter | The first user of given profile entered the given location.
5 | Profile location exit  | The first user of given profile left the given location.


<a name="mesh_result_packet"></a>
### Mesh result packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Stone ID | 1 | ID of the stone.
[Result packet](../docs/PROTOCOL.md#result_packet) | Result | N | The result.


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



