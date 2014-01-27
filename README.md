# Indoor Localization with BLE

The code base for this repository comes from [http://hg.cmason.com/nrf](http://hg.cmason.com/nrf). Thanks a lot Christopher!

Bluetooth LE (BLE) is not inherently fit for a wireless network. We have technology in-house that can do this [https://en.wikipedia.org/wiki/MyriaNed](Myrianed), but it didn't have wide acceptance yet. In general solutions such as ZigBee, Z-Wave, and other mesh solutions, will necessarily stay marginal accept if they get accepted in a common handheld.

That's why BLE is interesting. A lot of phones come with BLE, so a solution is automatically useful to a large variety of people. It is not the best technology for the job. The network topology is a Personal Area Network (PAN), not a Local Area Network (LAN). This means that you cannot have all nodes communicating with all other nodes at the same time. To get RSSI values we will have to set up connections to other nodes and tear them down again. Not very efficient. But it will do the job. Note that this considers a network with known nodes, so setting up connections will be relatively easy. On the moment I am not concerned with security so establishing some zeroconf method to detect which nodes belong to the network will not be part of this codebase. Feel free to clone.

## Todo list

* Clean up code
* Obtain RSSI values
* Set up management for establishing connections, getting RSSI values, and tear down connections again
* Create an algorithm to come up with all locations of the nodes. These positions are relative, not absolute. The network will be known up to rotation and scale.
* Implement this algorithm in a distributed fashion. The type of algorithm I have in mind is belief propagation / message passing.

## Copyrights

Obviously, the copyrights of the code written by Christoper, belong to him.

The copyrights (2013) for the rest of the code belongs to me and are provided under an noncontagious open-source license:

* Author: Anne van Rossum
* Date: 27 Jan. 2014
* License: LGPL v3
* Almende B.V., http://www.almende.com and DoBots B.V., http://www.dobots.nl
* Rotterdam, The Netherlands

