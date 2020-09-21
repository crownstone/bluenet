# UART Protocol 1.0
---------------

## Special characters

There are two special characters:
- Start: `0x7E`
- Escape: `0x5C`

Every byte in the UART message that equals one of the special characters will be XORed by `0x40`, and prepended by the escape character.


<a name="uart_wrapper"></a>
## UART wrapper

Every UART message is wrapped.

![UART wrapper](../docs/diagrams/uart_wrapper.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8   | Start          | 1 | Start of a message.
uint8   | Protocol major | 1 | Major protocol version, increased for breaking changes.
uint8   | Protocol minor | 1 | Minor protocol version, increased when new types are added.
uint8   | Message type   | 1 | Type of UART message, see below.
uint16  | Size           | 2 | Size of payload.
uint8[] | Payload        | N | Depends on message type.
uint16  | CRC            | 2 | The CRC16 (CRC-16-CCITT) of everything except the start byte.

Type | Payload
--- | ---
0 | [UART message](#uart_msg)
128 | [Encrypted UART message](#encrypted_uart_msg)


<a name="encrypted_uart_msg"></a>
### Encrypted UART message

Although the name suggests this is encrypted, only the `encrypted data` is actually encrypted. The other fields are unencrypted, but required for the encryption.

![Encrypted UART message](../docs/diagrams/encrypted_uart_msg.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8[] | Packet nonce | 3 | Packet nonce.
uint8   | Key ID       | 1 | Key ID used for encryption.
uint8[] | Encrypted data | N | Encrypted with [AES CTR](PROTOCOL.md#ctr_encryption). Get a (new) session nonce with the session nonce command.

<a name="encrypted_data"></a>
### Encrypted data

![Encrypted data](../docs/diagrams/uart_encrypted_data.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint32  | Validation   | 4 | Validation
uint16  | Messge size  | 2 | Size of the uart message in bytes.
uint8[] | [UART message](#uart_msg) | Size | The uart message.
uint8[] | Padding      | N | Padding to make this whole packet size a multiple of 16.


<a name="uart_msg"></a>
### UART message

![UART message](../docs/diagrams/uart_msg.png)

Type | Name | Length | Description
--- | --- | --- | ---
uint8   | Device ID    | 1 | User device ID for messages to the crownstone, or crownstone ID for messages from the crownstone.
uint16  | Data type    | 2 | Type of UART data.
uint8[] | Data         | N | The data packet, depends on type.



## TX data types (commands)

Data types for messages sent to the Crownstone.

- Each message will be replied to with a message with the same data type.
- Messages with _encrypted_ set to _yes_, have to be encrypted when the crownstone status has _encryption required_ set to true.
- Messages with _encrypted_ set to _optional_, may be encrypted.
- Types >= 50000 are for development. These may change, and will be disabled in release.

Type  | Data   | Encrypted | Description
----- | ------ | --------- | -----------
0     | -      | Never     | First command that is sent, used to determine whether this is the right crownstone, and whether encryption has to be used.
1     | [Session nonce](#cmd_session_nonce_packet) | Never | Refresh the session nonce.
2     | [Heartbeat](#cmd_heartbeat_packet) | Yes | Used to know whether the UART connection is alive.
3     | [Status](#cmd_status_packet) | Optional | Status of the user, this will be advertised by a dongle.
10    | [Control msg](../docs/PROTOCOL.md#control_packet) | Yes | Send a generic control command.
50000 | uint8  | Never | Enable/disable advertising.
50001 | uint8  | Never | Enable/disable mesh.
50002 | -      | Never | Get ID of this Crownstone.
50003 | -      | Never | Get MAC address of this Crownstone.
50103 | -      | Never | Increase the range on the current channel.
50104 | -      | Never | Decrease the range on the current channel.
50105 | -      | Never | Increase the range on the voltage channel.
50106 | -      | Never | Decrease the range on the voltage channel.
50108 | uint8  | Never | Enable/disable differential mode on current channel. (Currently the packet is ignored, and it toggles instead)
50109 | uint8  | Never | Enable/disable differential mode on voltage channel. (Currently the packet is ignored, and it toggles instead)
50110 | uint8  | Never | Change the pin used on voltage channel. (Currently the packet is ignored, and it rotates between certain pins instead)
50200 | uint8  | Never | Enable sending current samples.
50201 | uint8  | Never | Enable sending voltage samples.
50202 | uint8  | Never | Enable sending filtered current samples.
50204 | uint8  | Never | Enable sending calculated power samples.



## RX data types (events and replies)

Data types for messages received from the Crownstone.

- Messages with _encrypted_ set to _yes_, will be encrypted when the crownstone status has _encryption required_ set to true.
- Types in range 10000 - 20000 are events, not a (direct) reply to a UART command.
- Types in range 40000 - 50000 are for development. These may change, and will be enabled in release.
- Types >= 50000 are for development. These may change, and will be disabled in release.

Type  | Data   | Encrypted | Description
----- | ------ | --------- | ----
0     | [Hello](#ret_hello_packet) | Never | Hello reply.
1     | [Session nonce](#ret_session_nonce_packet) | Never | The new session nonce to use for encrypted messages sent by the user.
2     | -      | Yes | Heartbeat reply.
3     | [Status](#ret_status_packet) | Never | Status reply and event.
10    | [Result packet](../docs/PROTOCOL.md#result_packet) | Yes | Result of a control command.
10000 | string | Yes | As requested via control command `UART message`.
10002 | [Service data with device type](../docs/SERVICE_DATA.md#service_data_header) | Yes | Service data of this Crownstone (unencrypted).
10003 | [Service data without device type](../docs/SERVICE_DATA.md#service_data_encrypted) | Yes | State of other Crownstones in the mesh (unencrypted).
10004 | [Presence change packet](#presence_change_packet) | Yes | Sent when the presence has changed. Note: a profile ID can be at multiple locations at the same time.
10103 | [External state part 0](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_state_0_t) | Yes | Part of the state of other Crownstones in the mesh.
10104 | [External state part 1](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_state_1_t) | Yes | Part of the state of other Crownstones in the mesh.
10105 | [Mesh result](#mesh_result_packet) | Yes | Result of an acked mesh command. You will get a mesh result for each Crownstone, also when it timed out. Note: you might get this multiple times for the same ID.
10106 | [Mesh ack all result](../docs/PROTOCOL.md#result_packet) | Yes | SUCCESS when all IDs were acked, or TIMEOUT if any timed out.
50000 | Eventbus | Yes | Raw data from the event bus.
50103 | [Time](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_time_t) | Yes | Received command to set time from the mesh.
50110 | [Profile location](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_profile_location_t) | Yes | Received the location of a profile from the mesh.
50111 | [Behaviour settings](../docs/MESH_PROTOCOL.md#behaviour_settings_t) | Yes | Received command to set behaviour settings from the mesh.
50112 | [Tracked device register](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_device_register_t) | Yes | Received command to register a tracked device from the mesh.
50113 | [Tracked device token](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_device_token_t) | Yes | Received command to set the token of a tracked device from the mesh.
50114 | [Sync request](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_sync_request_t) | Yes | Received a sync request from the mesh.
50120 | [Tracked device heartbeat](../docs/MESH_PROTOCOL.md#cs_mesh_model_msg_device_heartbeat_t) | Yes | Received heartbeat command of a tracked device from the mesh.
50000 | uint8  | Never | Whether advertising is enabled.
50001 | uint8  | Never | Whether mesh is enabled.
50002 | uint8  | Never | Own Crownstone ID.
50003 | MAC    | Never | Own mac address (6 bytes).
50100 | [ADC config](#adc_channel_config_packet) | Never | ADC configuration.
50200 | [Current samples](#current_samples_packet) | Never | Raw ADC samples of the current channel.
50201 | [Voltage samples](#voltage_samples_packet) | Never | Raw ADC samples of the voltage channel.
50202 | [Filtered current samples](#current_samples_packet) | Never | Filtered ADC samples of the current channel.
50203 | [Filtered voltage samples](#voltage_samples_packet) | Never | Filtered ADC samples of the voltage channel.
50204 | [Power calculations](#power_calculation_packet) | Never | Calculated power values.
60000 | string | Never | Debug strings.



## Packets

<a name="ret_hello_packet"></a>
### Hello packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Sphere ID | 1 | Short sphere ID, as given during [setup](PROTOCOL.md#setup).
[status](#ret_status_packet) | Status | 1 | Status packet.


<a name="cmd_heartbeat_packet"></a>
### Heartbeat packet

Type | Name | Length | Description
--- | --- | --- | ---
uint16 | Timeout | 2 | If no heartbeat is received for _timeout_ seconds, the connection can be considered to be dead.


<a name="cmd_status_packet"></a>
### User status packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Type | 1 | Status type: 0=no-data, 1=crownstone-hub, 2-15=reserved, 16-255=free to use
uint8 | Flags | 1 | Standard flags: has_been_set_up, encryption_required, has_error, ...
uint8[] | Data | 8 | Status data to be advertised by dongle (will be ignored if status type is _no-data_).

<a name="ret_status_packet"></a>
### Crownstone status packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Flags | 1 | Flags: has_been_set_up, encryption_required, has_error, ...


<a name="cmd_session_nonce_packet"></a>
### Refresh session nonce packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8 | Timeout | 1 | How long (minutes) this session nonce is valid.
uint8[] | Session nonce | 5 | The session nonce to use for encrypted messages sent by the user.


<a name="ret_session_nonce_packet"></a>
### Session nonce reply packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8[] | Session nonce | 5 | The session nonce to use for encrypted messages sent by the crownstone.




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



