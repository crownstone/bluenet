# Add a new command

The command types can be found in the [Protocol specification](../protocol/PROTOCOL.md#command_types) document. To add a new 
command, expand the list in the documentation itself with a new type.

The list of commands can be found in [cs_CommandTypes.h](../../source/include/protocol/cs_CommandTypes.h). They are numbered

```
enum CommandHandlerTypes {
	...
	CTRL_CMD_YOUR_COMMAND = N,
}
```

You will likely want to **use** this command for something. You can define for this a data **type**. 
The following commands all make use of a type:

* `CTRL_CMD_GOTO_DFU` generates an `CS_TYPE::EVT_GOING_TO_DFU` event.
* `CTRL_CMD_SET_SUN_TIME` writes an `CS_TYPE::STATE_SUN_TIME` type.
* `CTRL_CMD_PWM` generates an `CS_TYPE::CMD_SET_DIMMER` event.
* `CTRL_CMD_RELAY` generates an `CS_TYPE::CMD_SET_RELAY` event.
* `CTRL_CMD_MULTI_SWITCH` generates an `CS_TYPE::CMD_MULTI_SWITCH` event.

That being said, there are exceptions, `CTRL_CMD_NOP` does nothing, it can be used as `keep-alive` command. There are
also a bunch of commands that handle the request directly. For example on `CTRL_CMD_GET_BOOTLOADER_VERSION` 
the data is obtained directly from RAM, on `CTRL_CMD_GET_UICR_DATA` it is obtained directly from UICR, on 
`CTRL_CMD_RESET` a `resetDelayed` function is called which calls `Timer::getInstance().start`, on
`CTRL_CMD_SET_TIME` the `SystemTime::setTime()` is directly called. Such a hard coupling is strongly 
**discouraged**. Use the event bus instead! If you are including header files to the 
[cs_CommandHandler.cpp](../../source/src/processing/cs_CommandHandler.cpp) you're likely doing something wrong.

## Data types

There is a large list of types in [cs_Types.h](../../source/include/common/cs_Types.h). There are three classes:

1. State types
2. Event types
3. Command types.

For example, when adding only a new `State type` you don't necessarily have to add a different type of payload to the 
protocol. There is a 
[state_set_packet](../protocol/PROTOCOL.md#state_set_packet) and a 
[state_get_packet](../protocol/PROTOCOL.md#state_get_packet) with a variable size payload.

If you do not want to have it stored as a state variable in `cs_State`, or if there are other reasons to define a 
custom command, you have to add it to `CS_TYPE`. 

```
enum class CS_TYPE: uint16_t {
	...
	CMD_YOUR_COMMAND = N,
}
```

This is a different index then in `CommandHandlerTypes` which is only used to represent the control opcodes for the 
protocol. The `CS_TYPE` can be seen as a something that allows you to:

* Write a `typedef` that indicates the format of this (command) type.
* Have the `TypeSize(CS_TYPE)` function return the size of this type.
* Have the `TypeName(CS_TYPE)` function return a readable type string for debugging purposes.
* Have a `hasMultipleIds(CS_TYPE)` function to allow storage of multiple versions of this type.
* Have a `removeOnFactoryReset(CS_TYPE)` function to indicate if persistence should go beyond a factory reset.
* Have a `getUserAcccessLevelSet(CS_TYPE)` function to indicate GET access level for this type.
* Have a `setUserAcccessLevelSet(CS_TYPE)` function to indicate SET access level for this type.

There are also further options that allow you to specify in [csStateData.cpp](../../source/src/storage/cs_StateData.cpp)
matters on (persistent) storage. You will have to indicate for non-state data (such as commands) that they are NOT
meant to be stored. This class allows you to:

* Have a `getDefault()` function that specifies the default value for a type (often the "factory default"). For a
command you'll often want to return `ERR_NOT_FOUND`.
* Have a `DefaultLocation` which is either `PersistenceMode::FLASH`, `RAM`, or `NEITHER_RAM_NOR_FLASH` (the latter in
case you do not want to store the type at all).

The reason that we have one over-arching type definition is that it makes it easy to send every possible type as an
event. Moreover, by specifying them as an `enum class` you will get compiler warnings if you forget to include them
in one of the large switch statements. Never add a `default` clause to those switch statements. Then we would not have
those compiler warnings anymore to prevent such mistakes.

The struct for the command you create can be specified in [cs_Packets.h](../../source/include/protocol/cs_Packets.h). An
example is:

```
struct __attribute__((packed)) session_nonce_t {
	uint8_t data[SESSION_NONCE_LENGTH];
};
```

You see that the struct is `packed`, important! Moreover, it is of fixed size. Always specify a maximum size!

## Communication

To add the new command to the part of the code where the BLE messages arrive, or more specific, where the command
messages arrive, navigate to [cs_CommandHandler.cpp](../../source/src/processing/cs_CommandHandler.cpp). There is the
`handleCommand` function that can be expanded.

You will have to do several things:

* Add `CTRL_CMD_YOUR_COMMAND` as valid option in the start.
* Add in the function `getRequiredAccessLevel` the access level, `ENCRYPTION_DISABLED`, `BASIC`, `MEMBER`, or `ADMIN`.
* Then handle the command itself, see next.

You can handle the command by adding a case statement:

```
void CommandHandler::handleCommand(...)
	...
	case CTRL_CMD_YOUR_COMMAND:
		return handleYourCommand(commandData, accessLevel, result);
```

Or you can immediately dispatch an event by:

```
void CommandHandler::handleCommand(...)
	...
	case CTRL_CMD_YOUR_COMMAND:
		return dispatchEventForCommand(CS_TYPE::CMD_YOUR_COMMAND, commandData, result);
```

Note the subtle change from the opcode `CTRL_CMD_YOUR_COMMAND` to the type `CS_TYPE::CMD_YOUR_COMMAND`.

When the command is send through as an event, the `dispatch()` function will synchronously update all event listeners.
However, those event listeners can have asynchronous calls. For example, writing something to flash and receiving a
"done" request after this operation has finished. The `result.returnCode`, `.data`, and `.dataSize` fields can 
therefore not always be written immediately.

For this you will need to use notifications.

## Asynchronous commands

When your command is asynchronous, it should return `WAIT_FOR_SUCCESS`. This will block other commands from being processed, so it should not actually take that long.
Once the command is done, dispatch the event `CMD_RESOLVE_ASYNC_CONTROL_COMMAND` with the result. The command handler cached the source and will decide whether to send the result via BLE or UART.

An example of a command doing this is `CTRL_CMD_SETUP`. The Setup class starts writing to flash and returns `WAIT_FOR_SUCCESS`. Then it waits for the writes to be done. Once the writes are done, it resolves the control command.

An example with data in the final result is the command `CTRL_CMD_HUB_DATA`. The data is sent to a hub via uart, and the user has to wait for the hub to reply via uart. Once the reply is received, the UartCommandHandler resolves the control command.

## Background: implementation of result notifications

The [CrownstoneService](../../source/src/services/cs_CrownstoneService.cpp) is where the characteristics are defined. See
also the protocol [document](../protocol/PROTOCOL.md#crownstone-service). The `_controlCharacteristic` is the characteristic that 
is used to send the new command (of above) to the Crownstone. If you inspect the code you see that it sets `setNotifies(false)`. It has a different
characteristic to communicate results back, the `_resultCharacteristic`. This has `setNotifies(true)`.

The results can be written to this characteristic through `writeResult(CommandHandlerTypes, cs_result_t)`. It will
write:

```
_resultPacketAccessor->setPayloadSize(result.dataSize);
_resultPacketAccessor->setType(type);
_resultPacketAccessor->setResult(result.returnCode);
_resultCharacteristic->setValueLength(_resultPacketAccessor->getSerializedSize());
_resultCharacteristic->updateValue();
```

If you pay close attention you see that the same `_resultPacketAccessor` is used in the function
`_controlCharacteristic->onWrite`. That's why there is no actual data in the notification. The only fields that
are set in `writeResults` are `type`, `result.dataSize`, and `result.returnCode`. If you actually want to write
the data you will have to access through `_resultPacketAccessor` or `readBuf.data`. The latter you can reach through
`CharacteristicReadBuffer::getInstance().getBuffer()`.

Note that on a write, for a while the `result.buf.data` buffer in `_resultPacketAccessor` will be available for the 
`handleCommand()` function. If there is an incoming write while we are waiting for an asynchronous `writeResults`
call, this can overwrite what's in that buffer through a second call to `handleCommand()`.
