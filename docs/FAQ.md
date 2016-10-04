# FAQ

### Encryption

#### Why don't you use the Bluetooth Bonding?

Bluetooth bonding in Bluetooth V4 creates a secure link between user and device. There are a few downsides of this method which we will explain here:

- Before Bluetooth 4.2 (which introduces Elliptical Curve Hellman-Diffie) the initial exchange of the keys is not secure.
- Bluetooth 4.2 is not supported on the majority of phones at the time of writing.
- Every user needs to bond with each Crownstone.
- Every Crownstone stores a key for each user with whom it has bonded, thereby limiting the total amount of possible users.
- The secure bonding does not secure the scan response advertisements.

#### Why not use Apple Homekit?

This is on the roadmap for future releases and will work with the currently available Crownstones but at the moment Apple Homekit is not supported on Android.