# Breaking change wishlist
Document to keep up a list of changes we want, but don't implement because they would break protocol.


## BLE connection

- Remove encrypted session data characteristic: this can be done unencrypted (characteristic is already there).

### Multiple connections:
- Replace the validation key with a CRC32 over the encrypted payload (including padding). This allows us to check data integrity after copying the data on write. We need this because the write buffer can be written at any moment.
- Add a connection handle to the session data, so the user knows its connection handle.
- Add a connection handle to the result data, so the user can filter out results not aimed at them.
