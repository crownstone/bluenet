# Service and characteristics

This document describes how services and characteristics are implemented in bluenet.


## Services

Every service can add multiple characteristics. It should do so in `createCharacteristics()`.
The characteristics should use the service UUID as base.

## Characteristics

There are many different characteristic classes, each for a different value type.
These classes are specializations of the templated CharacteristicGeneric. All classes are derivatives of CharacteristicBase.

Right now, only a few different types are used:
- `Characteristic<buffer_ptr_t>`
- `Characteristic<uint32_t>`
- `Characteristic<std::string>`

## Buffers

Every characteristic needs a read and/or write buffer. This buffer is used by the softdevice, and is what the connected device reads or writes. In the code, these are often called "gattValue".

Because the buffers that the connected device can read or write should be encrypted, these buffers should not be the same as the unencrypted buffers.

Which buffers are used, is determined by the service implementation, and the type of characteristic.

To reduce the amount of buffers, there are some buffers that are reused. CrownstoneCentral can use them as well, since there can be only 1 connection: either outgoing or incoming. It is still important though, not to write unencrypted data to the encryption buffer.

### CharacteristicWriteBuffer

Initialized by Stack.

Used by:
- Stack
    - For incoming long writes, this buffer is returned when the softdevice requests memory.
- Control characteristic
    - For incoming writes. I believe this matches the softdevice memory request.
- CrownstoneCentral
    - For outgoing writes, since a write goes in chunks, and thus the data to write must be retained.


### CharacteristicReadBuffer

Initialized by Stack.

Used by:
- Result characteristic
- Session data characteristic
- CrownstoneCentral
    - To decrypt the merged incoming notifications to.

### EncryptionBuffer

Initialized by Crownstone.

Used by:
- CrownstoneCentral
    - To merge multiple incoming notifications to.
- BleCentral
    - To read and write (because we usually read and write encrypted data).
- Any characteristic with `(_minAccessLevel < ENCRYPTION_DISABLED)`, `_status.sharedEncryptionBuffer`, and `_status.aesEncrypted`.

## Usage and implementation details

- When `sharedEncryptionBuffer` is false, a separate encryption buffer will be allocated.
- updateValue() will encrypt the value of a characteristic to the encrypted buffer. Also, when notifications are enabled, it will start notifying the value.

## Notifications

When the value is updated, it can be notified in chunks, as notifications are of limited size. Each time, onTxComplete, the next chunk is notified.
This means that the gatt value buffer is read until the last part is notified.

## Long write

When large buffer is written to a characteristic, it's called a long write.
A long write is written in chunks by multiple "prepare write requests", then finalized by "execute write request". See this [sequence diagram](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s132.api.v6.1.1%2Fgroup___b_l_e___g_a_t_t_s___q_u_e_u_e_d___w_r_i_t_e___b_u_f___n_o_a_u_t_h___m_s_c.html).

When the first request is received, the softdevice will request memory to store the different requests.
Multiple characteristics can be written with a single long write, which is why the execute write request event does not contain an characteristic handle.

Right now, bluenet assumes only 1 characteristic was written.
Also, bluenet gets the handle from the raw buffer (see [memory layout](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s132.api.v5.0.0%2Fgroup___b_l_e___g_a_t_t_s___q_u_e_u_e_d___w_r_i_t_e_s___u_s_e_r___m_e_m.html&cp=2_3_1_1_0_2_4_5)) that was given on memory request. The latter could be avoided by using `sd_ble_gatts_value_get()` instead.
