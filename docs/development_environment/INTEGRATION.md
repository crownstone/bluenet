Getting Started Guide to use Crownstone tech
===================

Crownstone tech can be used to enrich smart home and smart building solutions, build indoor positioning and proximity systems, unlock Bluetooth low energy devices through the Crownstone bluetooth mesh, recognize daily activities, and much more. This document is meant as a Getting Started Guide to experiment, demo, and integrate with Crownstone tech.

Integration can be done on multiple levels.

- [Control Crownstones via REST.](#control-via-rest)
- [Control Crownstones from within your own app.](#control-via-own-app)
- [Control Crownstones from your own hardware.](#control-via-own-hardware)
- [Obtain data from the Crownstones.](#read)
- [Provide indoor localization functionality using the Crownstone app.](#localization-via-rest)
- [Provide indoor localization functionality using your own app.](#localization-via-own-app)
- [Track wearables.](#track)
- [Identify devices.](#device-identification)


=====
A Crownstone [REST API](#rest-api) is available that uses the official Crownstone app to control devices that are in range of the phone of the user.

=====
There are two options:

- You can use our [Android](https://github.com/crownstone/bluenet-lib-android) and [iOS](https://github.com/crownstone/bluenet-ios-lib) libs to control Crownstones from your app.
- You can use our [React Native app](https://github.com/crownstone/CrownstoneApp) and integrate it with your RN app


=================
Your hardware needs to have a Bluetooth low energy (BLE) radio or a USB port in which a Bluetooth low energy dongle can be plugged. The hardware needs to be able to run software that transforms requests towards commands that are sent over BLE. 

Support from Crownstone exists on three levels:

### BLE-Python interface
A Python library will be provided on github (not yet available). It has an interface in the form of a Python class and will be available as module via pip. 

A Crownstone, when bought in retail stores, is in a pristine factory state. The procedure that will allow secure communication with the Crownstones requires a key on the Crownstones. The key exchange process can be performed by the Crownstone app or by your own software.

If the key has been created by our cloud, you can [retrieve the keys](#keys).

If the key is created in your own system the Crownstone cloud nor app will be able to control the Crownstones. If you would like your users to make use of the standard Crownstone app, you will have to upload the key to the Crownstone cloud. The data structures with respect to rooms, users, spheres, will also need to be synchronized between your own database and Crownstone via [REST](#rest-api) if you want to make use of the standard Crownstone app as well as your own. The encryption key is coupled to a sphere. More on security can be read [here](SECURITY.md).

### BLE-Websocket interface
A (language-agnostic) binary will be provided on github (not yet available). It has an interface in the form of a Websocket interface.

### Crownstone REST API
A Crownstone [REST API](#rest-api) is available that uses the official Crownstone app to control devices that are in range of the phone of the user. 

====
All data sent from the Crownstones is safely encrypted. If you want to be able to read the information, you will need the [keys](#keys).

If you want to download the information from the cloud, use the [REST interface](#rest-api).

If you want to read out the information from the Crownstones using a computer or raspberry pi, use the python lib, if you want to read it out through a phone, use the phone libs.


======
You can use the [REST interface](#rest-api) to subscribe to a location change event.


=== 
You will have to use our [Android](https://github.com/crownstone/localization-lib-android-basic) or [iOS](https://github.com/crownstone/bluenet-ios-basic-localization) indoor localization lib, provide your own UI for training locations and use our training helper libs to train these locations. You can optionally use our bluenet libs to get the ibeacon data and/or advertisements from the Crownstones. These can also be obtained though your own code. The indoor localization libs will provide you with a location per sample you give them.

===
Soon.

====
Soon.


====
You need keys to read and control Crownstones. The keys can be retrieved at the [website](https://my.crownstone.rocks/), or through [OAUTH2](https://github.com/crownstone/crownstone-sdk/blob/master/REST_API.md#oauth2).

Currently you first log in to the [cloud](https://cloud.crownstone.rocks/), then fill in the token at the [API explorer](https://crownstone-cloud.herokuapp.com/explorer).
After that, get your user id, with a [GET on /users/me](https://crownstone-cloud.herokuapp.com/explorer/#!/user/user-me).
Finally, get your keys with a [GET on /users/{id}/keys](https://crownstone-cloud.herokuapp.com/explorer/#!/user/user-getEncryptionKeys).

====
You can control and read out Crownstones via the [REST API](https://github.com/crownstone/crownstone-sdk/blob/master/REST_API.md).
The cloud data can be accessible to external developers by using the [OAUTH2](https://github.com/crownstone/crownstone-sdk/blob/master/REST_API.md#oauth2) interface, or to users through the REST explorer.

With access to the Cloud, you can register webhooks to certain events. For instance, if someone changes location (goes from room to room), you can register an endpoint that will be called by our cloud with the changed data. You can use these hooks to do a lot of neat things! Listen to the power usage to check if everything is OK, get a notification when a light turns on, welcome your user when they enter their home etc, turn on any other IOT device in your platform when a user enters the room etc.

This requires the official Crownstone app to be running and the Crownstones to be in range of phone or hub running the app.
The app should also be set to allow uploading of information, and act as hub to control Crownstones.

