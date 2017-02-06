* KEEP ALIVE

1 2   3                       4

ID  | action + switch state | timeout |

switch state = 0 - 100
do nothing = 255
don't care ?


> encrytpion

1 2 3 4           5 6 7 ...

message counter | encrypted payload
(unencrypted)
                | 5 6 7 8           9 10   11                      | 12      | 13 ...

                | message counter | ID   | action + switch state + | timeout | ID ...


* STATE BROADCAST

every x seconds

element: 

1 2  3              4 5 6 7       8 9 10 11

ID | switch state | power usage | accumulated energy usage

array:

1      2      3 ...

HEAD | TAIL | [ ELEMENT ]

8 element per message


* STATE CHANGE BROADCAST

same as STATE BROADCAST, added/triggered every time the state changes significantly, e.g. power usage changes a lot, or switch state changes


* USER COMMAND

1 2    3     4 5 ...

type | num | [ID]x num | payload


* COMMAND REPLY

1 2    3 4 5 6           7 ...

type | message counter | payload

                         7     8 9   10 ..

                         num | ID  | data | ID | data ...


* SCAN RESULT

element:

ID | address | rssi

array:

[ element ]

or

array:

ID | address | rssi | address | rssi | ...

alternatively hub requests scans in turn


* LARGE DATA

multi part messages