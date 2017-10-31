Getting Started Guide to use Crownstone tech
===================

Crownstone tech can be used to enrich smart home and smart building solutions, build indoor positioning and proximity systems, unlock Bluetooth low energy devices through the Crownstone bluetooth mesh, recognize daily activities, and much more. This document is meant as a Getting Started Guide to experiment, demo, and integrate with Crownston tech.

Integration can be done on multiple levels.

1 - Control Crownstones from your own hardware.
2 - Control Crownstones from within your own app.
3 - Obtain data from the Crownstones.
4 - Provide indoor localization functionality using the Crownstone app.
5 - Provide indoor localization functionality using your own app.
6 - Track wearables.
7 - Identify devices.

1 - Control Crownstones from your own hardware
=================

Your hardware needs to have a Bluetooth low energy (BLE) radio or a USB port in which a Bluetooth low energy dongle can be plugged. The hardware needs to be able to run software that transforms requests towards commands that are sent over BLE. 

Support from Crownstone exists on three levels:

a. BLE-Python interface
A Python library will be provided on github (not yet available). It has an interface in the form of a Python class and will be available as module via pip. 

A Crownstone if bought in retail stores is in a pristine factory state. The procedure that will allow secure communication with the Crownstones requires a key on the Crownstones. The key exchange process can be performed by the Crownstone app or by your own software. 

Implementation detail note. If the key is created in your own system the Crownstone cloud nor app will be able to control the Crownstones. If you would like your users to make use of the standard Crownstone app, you will have to upload the key to the Crownstone cloud. The data structures with respect to rooms, users, spheres, will also need to be synchronized between your own database and Crownstone if you want to make use of the standard Crownstone app as well as your own. The encryption key is coupled to a sphere. More on security can be read at https://github.com/crownstone/bluenet/blob/master/docs/SECURITY.md.

b. BLE-Websocket interface
A (language-agnostic) binary will be provided on github (not yet available). It has an interface in the form of a Websocket interface.

c. Crownstone REST API
A Crownstone REST API is available that uses the official Crownstone app to control devices that are in range of the phone of the user. 

2 - Control Crownstones from within your own app
=====

- You can use our React Native app and integrate it with your RN app
- You can use our Android and iOS libs to control Crownstones from your app


3: Read out information from Crownstones
====

All data sent from the Crownstones is safely encrypted. If you want to be able to read the information, you will need the keys. These can be obtained from the Cloud through OAUTH2 or the explorer.

If you want to download the information from the cloud, use the REST service.

If you want to read out the information from the Crownstones using a computer or raspberry pi, use the python lib, if you want to read it out through a phone, use the phone libs.


4: use indoor localization from the Crownstone app to trigger my things 
======

You can use the REST interface to subscribe to a location change event.


5: use indoor localization with my own app running on a phone 
=== 

You will have to use our indoor localization libs, provide your own UI for training locations and use our training helper libs to train these locations. You can optionally use our bluenet libs to get the ibeacon data and/or advertisements from the Crownstones. These can also be obtained though your own code. The indoor localization libs will provide you with a location per sample you give them.

6: Track wearables
===
We can't, yet.

7: Identify devices
====
We can't, yet.


The REST interface
====================

If you use the official Crownstone app, and the user allows the uploading of information, this information is stored in the Crownstone Cloud. This cloud data can be accessible to external developers by using the OAUTH2 interface, or to users through the REST explorer.

With access to the Cloud, you can register webhooks to certain events. For instance, if someone changes location (goes from room to room), you can register an endpoint that will be called by our cloud with the changed data. You can use these hooks to do a lot of neat things! Listen to the power usage to check if everything is OK, get a notification when a light turns on, welcome your user when they enter their home etc, turn on any other IOT device in your platform when a user enters the room etc.
