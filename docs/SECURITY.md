# Intro

First, if you have never read up on security, please watch some movies online on [Defcon](https://www.defcon.org/). 
On security and BLE we recommend [this talk](https://media.ccc.de/v/33c3-8019-lockpicking_in_the_iot) by Ray. You will be quickly 
uptodate about all necessary tools, such as the ST-Link, Ubertooth, wireshark, smali, etc. 

Crownstone approaches security from the perspective that YOU should be the OWNER of your devices including the firmware and the
keys that come with it (see [blog post](https://crownstone.rocks/2017/01/07/the-devilish-dilemma-of-supporting-android-and-ios-with-ble)). 
We are against security by obscurity. Note, for example that this means we at times do not integrate with third-party vendors 
if they hide their sloppy implementation under an NDA! 

| Security feature                  | Description                                                            |
| ---                               | ---                                                                    |
| Message encryption                | Every message, including advertisements are encrypted with AES128      |
| Usage leakage protection          | Each plug sends out advertisements, even if nothing happens            |
| Replay attack protection          | There are counters involved that make replay attacks much harder       |
| Close-range setup                 | Minimize sniffing opportunities at the setup process                   |
| Multiple keys                     | There are several keys for different levels of access                  |

## Message encryption

AES is a symmetric cypher. This means that it applies a bunch of XORs that flip bits from ones to zeros and the other way around. 
On embedded hardware this is space and memory preserving because you can use the same functions for encryption and decryption.
You can read more on AES on [Wikipedia](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard). Using 128 bits is considered 
sufficient in the USA for information up to level SECRET. Nice note to the future quantum hacker, symmetric keys such as AES seem to be 
[quantum resistant](https://en.wikipedia.org/wiki/Post-quantum_cryptography).

## Usage leakage protection

If our devices would only communicate if their state would change, it would inadvertently reveal information about the presence
of someone at home, or their use of their devices. 

## Replay attack protection

If someone captures a message that is intended for one of your switches/plugs and replays it a later time while you're not
home for example, they will not be able to switch a light or repeat the action by just resending the capture message.

## Close-range setup

The setup process exchanges keys between the smartphone of the admin and the Crownstone device itself. The signal strength of 
the radio is severely reduced before the Crownstone is set up. This means that while you are configuring your devices the
person listening in on that process must basically sit next to you at that specific (short) time window.

## Multiple keys

A device does have multiple keys. Not only the connections themselves, also the advertisements are encrypted with the keys on the device. The nodes that belong together in a network are organized in so-called "spheres". Hence, each sphere has a unique security key. The key that allows a user to decrypt the advertisements in a sphere is called the **guest key**. This means that a user with this access level can control the devices, they can also see the power consumption of devices and other state info. This key is the one that is exchanged with friends. It has to be seen as that key that will allow friends to control the devices just as in the case they are physically present. This key is also sufficient to decrypt the advertisements required for indoor localization. So, also for your friends the lights will remain on if they are in the living room. Another key, the **admin key** allows people to add guests to the household. The admin organizes most things around access and is typically the person who's the first user of the Crownstone system. There is an intermediate **member key**. This is a key typical for other household members, who can for example set schedules for devices. The detailed permission levels can be read in the [Protocol doc](https://github.com/crownstone/bluenet/blob/master/docs/PROTOCOL.md).

# Considerations

There are - regretfully - tradeoffs to make between security and usability.

| Security-related decision         | Description                                                            |
| ---                               | ---                                                                    |
| BLE 4.0                           | iPhone 5 only supports 4.0 and has too big a market share              |
| Passwords                         | Storage of hashed passwords in the app                                 |
| Factory reset                     | Physical access means ability to factory reset                         |
| Firmware                          | Firmware checksum                                                      |

## BLE 4.0

The setup process with BLE 4.2 can use [Elliptic curve Diffie-Helmann](https://en.wikipedia.org/wiki/Elliptic_curve_Diffie–Hellman).
In other words, the proper way to exchange keys over an unsafe channel. However, the iPhone 5 only supports 4.0 and has a 
market share too big to ignore. For that reason we use a close-range setup process as explained above. As soon as BLE 4.2 is
supported by all smartphone models we will be happy to migrate.

## Passwords

At first we only stored a token in the app. A token is obtained by a request to the cloud servers with username and password. 
However, if we invalidate tokens after a new username and password request this means that the simultaneous use of multiple
devices or the simultaneous use of a device and a dashboard means that the user has to login time after time.

We therefore opted for storing the password in a hashed form in the app. It is the responsibility of the device owner not to 
install malicious apps that try to read across app boundaries on their smartphone / tablet. However, even so, the plain password 
will not be leaked. 

## Factory reset

In our consumer products physical proximity means that the user is able to perform a factory reset. This is the most difficult
security decision we had to make. In the end our reasoning has been the following: if someone is close enough to steal the
device, they might as well be able to factory reset it. If that person subsequently reconfigures the device, it will have the
wrong keys, so the rightful owner will notice if someone tampered with their devices. We might remove this function in the
future, but then people run the risk in bricking their own devices by loosing their keys. If someone has a good suggestion for 
this scenario that is convenient and secure at the same time, contact us!

Think of the following scenarios:

+ a divorce gone wrong, and although he left, he locked you out from using your own devices/lights;
+ losing your smartphone plus the login information to the cloud;
+ bringing your own devices to your new home.

Note that the time window in which someone can perform a factory reset is very limited. It is only within around 30 seconds after the Crownstone is powered on. You won't be able to do it by just being close alone, you have to switch off the power to that Crownstone as well.

## Firmware

There is currently no firmware checksum applied. We are of the opinion that you should be able to put whatever firmware you
like on our devices. 

# There are TODOs!

Yes, there are TODOs. 

| Security todos                    | Description                                                            |
| ---                               | ---                                                                    |
| Retract access                    | On retracting a guest access key, this key needs to be renewed         |

## Retract access

Suppose you have given your friend a key, but your friend is your friend not anymore. This means that this key needs to be 
renewed if that same key is used by a group of your friends. The alternative would be separate keys for any person, however, 
that does not scale up if you have many friends. :-) 

Moreover, we have to encrypt the advertisements with the same key or else no one will be able to read them except for the 
owner. Hence, the only logical solution is to re-roll the encryption keys on every mutation of the guest list.
