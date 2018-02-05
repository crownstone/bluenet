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
10000 | uint8  | Enable/disable advertising. (Currently the packet is ignored, and it toggles instead)
10001 | uint8  | Enable/disable mesh. (Currently the packet is ignored, and it toggles instead)
10103 | -      | Increase the range on the current channel.
10104 | -      | Decrease the range on the current channel.
10105 | -      | Increase the range on the voltage channel.
10106 | -      | Decrease the range on the voltage channel.
10108 | uint8  | Enable/disable differential mode on current channel. (Currently the packet is ignored, and it toggles instead)
10109 | uint8  | Enable/disable differential mode on voltage channel. (Currently the packet is ignored, and it toggles instead)
10110 | uint8  | Change the pin used on voltage channel. (Currently the packet is ignored, and it rotates between certain pins instead)
10200 | uint8  | Enable sending current samples. (Currently the packet is ignored, and it toggles instead)
10201 | uint8  | Enable sending voltage samples. (Currently the packet is ignored, and it toggles instead)
10202 | uint8  | Enable sending filtered current samples. (Currently the packet is ignored, and it toggles instead)
10204 | uint8  | Enable sending calculated power samples. (Currently the packet is ignored, and it toggles instead)





