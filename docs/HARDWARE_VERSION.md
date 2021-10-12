# Hardware version

See also the [product naming](https://github.com/crownstone/bluenet/blob/master/docs/PRODUCT_NAMING.md) document.


```
---------------------------------------------------------------
| GENERAL | PCB      | PRODU | HOUS  | RESERVED    | NORDIC   |
| PRODUCT | VERSION  | CTION | ING   |             | CHIP     |
| INFO    |          | RUN   |       |             | VERSION  |
---------------------------------------------------------------
| 1 01 02 | 00 92 00 | 00 00 | 00 00 | 00 00 00 00 | QF AA BO |
---------------------------------------------------------------
  |  |  |    |  |  |    |  |    |  |    |  |  |  |    |  |  |---  Build Code
  |  |  |    |  |  |    |  |    |  |    |  |  |  |    |  |------  Variant Code
  |  |  |    |  |  |    |  |    |  |    |  |  |  |    |---------  Package Code
  |  |  |    |  |  |    |  |    |  |    |--|--|--|--------------  Reserved for future use
  |  |  |    |  |  |    |  |    |--|----------------------------  Reserved for housing
  |  |  |    |  |  |    |  |------------------------------------  Week number of production run
  |  |  |    |  |  |    |---------------------------------------  Year number (last two digits) of production run
  |  |  |    |  |  |--------------------------------------------  Patch number of PCB version
  |  |  |    |  |-----------------------------------------------  Minor number of PCB version
  |  |  |    |--------------------------------------------------  Major number of PCB version
  |  |  |-------------------------------------------------------  Product Type: 1 Dev, 2 Plug, 3 Builtin, 4 Guidestone, 5 USB dongle, 6 Builtin One
  |  |----------------------------------------------------------  Market: 1 EU, 2 US
  |-------------------------------------------------------------  Family: 1 Crownstone
```

```
----------------------------------------------
| GENERAL | ASSEMBLY        | PRODU | PIECE  |
| PRODUCT |                 | CTION | NUMBER |
| INFO    |                 | RUN   |        |
----------------------------------------------
| 1 01 02 | 0 0 0 0 0 0 0 0 | 00 00 | 000000 |
----------------------------------------------
  |  |  |   | | | | | | | |    |  |        |---  Incremental number of piece within production run
  |  |  |   | | | | | | | |    |  |------------  Week number of production run
  |  |  |   | | | | | | | |    |---------------  Year number (last two digits) of production run
  |  |  |   | | | | | | | |--------------------  Packaging assembly line
  |  |  |   | | | | | | |----------------------  Product (Plastic + Metal + PCB) assembly line
  |  |  |   | | | | | |------------------------  Plastic + Metal assembly line
  |  |  |   | | | | |--------------------------  Metal manufacturing line
  |  |  |   | | | |----------------------------  Plastic manufacturing line
  |  |  |   | | |------------------------------  PCB with metal assembly line
  |  |  |   | |--------------------------------  PCB pick & place assembly line
  |  |  |   |----------------------------------  PCB manufacturing line
  |  |  |--------------------------------------  Product Type: 1 Dev, 2 Plug, 3 Builtin, 4 Guidestone, 5 USB dongle, 6 Builtin One
  |  |-----------------------------------------  Market: 1 EU, 2 US
  |--------------------------------------------  Family: 1 Crownstone
```
