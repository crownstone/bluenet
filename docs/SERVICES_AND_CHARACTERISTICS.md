# Service and characteristics

This document describes how services and characteristics are implemented in bluenet.

An introduction to BLE services and characteristics can also be found [here](https://devzone.nordicsemi.com/guides/short-range-guides/b/bluetooth-low-energy/posts/ble-characteristics-a-beginners-tutorial).

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

Because the buffers that the connected device can read or write should be encrypted, the characteristics use 2 buffers: one encrypted (gattValue), and one plain text (value).

Which buffers are used, is determined on characteristic creation (imjplemented in the service), and the type of characteristic.

Characteristics of basic types (integers), or string type, simply have the value as member variable and the value buffer is a pointer to that value. By default, these characteristics allocate an encrypted buffer themselves.
Characteristics of buffer type, need to be provided a buffer on creation. By default, these characteristics share the same encrypted buffer.

### Shared buffers

To reduce the amount of buffers, there are some buffers that are reused. CrownstoneCentral can use them as well, since there can be only 1 connection: either outgoing or incoming.
It is still important though, not to write plain text data to the encrypted buffer, because the user can read a characteristic that uses this uncrypted byffer at any time,

Buffers that are reused:

#### CharacteristicWriteBuffer

Initialized by Stack.

Used by:
- Control characteristic
    - To decrypt incoming writes to.
- CrownstoneCentral
    - For outgoing writes, since a write goes in chunks, and thus the data to write must be retained.


#### CharacteristicReadBuffer

Initialized by Stack.

Used by:
- Result characteristic (as plain text buffer, not as gatt value buffer)
- Session data characteristic (as plain text buffer, not as gatt value buffer)
- CrownstoneCentral
    - To decrypt the merged incoming notifications to.

#### EncryptedBuffer

Initialized by Crownstone.

Used by:
- Any encrypted characteristic
    - That is, if: `(minAccessLevel < ENCRYPTION_DISABLED)`, `sharedEncryptionBuffer`, and `aesEncrypted`.
- Stack
    - For incoming long writes, this buffer is returned when the softdevice requests memory.
- CrownstoneCentral
    - To merge multiple incoming notifications to.
- BleCentral
    - To read and write (because we usually read and write encrypted data).

## Notifications

When the value is updated, it can be notified in chunks, as notifications are of limited size. Each time, onTxComplete, the next chunk is notified.
This means that the gatt value buffer is read until the last part is notified.

## Long write

When large buffer is written to a characteristic, it's called a long write.
A long write is written in chunks by multiple "prepare write requests", then finalized by "execute write request". See this [sequence diagram](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s132.api.v6.1.1%2Fgroup___b_l_e___g_a_t_t_s___q_u_e_u_e_d___w_r_i_t_e___b_u_f___n_o_a_u_t_h___m_s_c.html).

When the first request is received, the softdevice will request memory to store the different requests.
Multiple characteristics can be written with a single long write, which is why the execute write request event does not contain an characteristic handle.

Right now, bluenet assumes only 1 characteristic was written.
Also, bluenet gets the characteristic handle from the raw buffer (see [memory layout](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s132.api.v5.0.0%2Fgroup___b_l_e___g_a_t_t_s___q_u_e_u_e_d___w_r_i_t_e_s___u_s_e_r___m_e_m.html&cp=2_3_1_1_0_2_4_5)) that was given on memory request.
The right way to do this would be to store all the characteristic handles from the prepare write requests.

## Usage

TODO: use control command as example. Add sequence diagram.

### Implementation

Misc:

- When `sharedEncryptionBuffer` is false, a separate encryption buffer will be allocated.
- updateValue() will encrypt the value of a characteristic to the encrypted buffer. Also, when notifications are enabled, it will start notifying the value.

When the user writes to a characteristic:

- `Stack::onwrite()`
    - Extracts the characteristic handle from the raw buffer.
- `Service::onWrite(handle)`
    - Calls `sd_ble_gatts_value_get()` with NULL as buffer, to get the length that was written.
- `CharacteristicGeneric::onWrite(length)`
    - Sets gatt value length to given length.
    - Sets the value length based on the gatt value length.
    - Decrypts the gatt value.
    - Checks access level.
    - Calls the user callback with the value as data.


