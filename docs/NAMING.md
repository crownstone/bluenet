# Naming Convention

broad product line | product line | product | part | version i.e. 00 000000 00 X_Name 0.0.0

## Product families

Two digits.

    01: Crownstone 'family'
    02: Third-party 'family'

## Product lines within each family

Six digits in binary format. This means that parts can be reused over multiple products.

### Crownstone family

    000001 Development Board     01
    000010 Plug                  02
    000100 Built-in              04
    001000 (Own) Guidestone      08

### Third-party family

    000001 Guidestone            01

## Market

    01 European (Schuko) version
    02 North American version

## Parts

    X:
      M Metal part
      P Plastic Part
      E Electronics related part (Circuit board etc.)
    Name:
      Plug_Part (the semisphere of the Guidestone and the Crownstone plug that is inserted into existing electrical sockets)
      Plug_cap_part (the insert of the plug part containing the actual electrical contact meterials, pins, ground plate etc.)
      Socket_Part (the top semisphere of the Crownstone plug that accepts other electrical plugs into itself)
      Beacon_cap (the top with eventually a logo of the Guidestone)
      Ring (the ring separating both hemispheres on both the Guidestone and the Crownstone plug)
      Pin (one of the two individual pins of a plug that fits into a C, E, or F power socket)
      Pin_receptor (receptacle on the PCB that tightly fits European plugs, only on the Crownstone plug)
      Ground_clip (part of the socket, attaching to the Socket_part of the Crownstone)
      Ground_plate (part of the plug, attaching to the Plug_cap_part of the Crownstone\Guidestone)

## Version

The version (#1.#2.#3) (can run higher than 9 per digit i.e. 1.12.106, but should be avoided)

    #1: major revision or release
    #2: minor revision
    #3: tweak or bugfix

## Date of creation

Optionally you can append the date of creation to the end of the filename, or the date of last edit. Preferably in "6-number form" i.e. 010316 (or 01-03-16) = march 1st 2016

# Part list

Current part list for the Crownstone Plug and Guidestone:

    01 000010 01 P_Plug_part
    01 000010 01 P_Plug_cap_part
    01 000010 01 M_Pin_receptor

    01 001000 01 P_Beacon_cap
    01 001000 01 M_Ground_plate

    01 001010 01 P_Socket_part
    01 001010 01 P_Ring
    01 001010 01 M_Pin
    01 001010 01 M_Ground_clip

We need 1 board for the Guidestone and 4 for the Plug?

# Vendor product number

The product lines can also be used directly for a vendor product number (VPN). This number is communicated with
distributors. It changes for each type of packaging.

| Name                             | Description                                        | Certification | Variant   | VPN         |
| --                               | --                                                 | --            | --        | --          |
| Crownstone Built-in Kit Zero     | Professional kit with three built-in Crownstones   | CE            | 240V      | CR101M01/01 |
| Crownstone Built-in Kit One      | Professional kit with three built-in Crownstones   | CE & UL       | 110/240V  | CR101M02/02 |
| Crownstone Plugs Kit             | Ready-to-go kit with two Crownstone plugs          | CE            | Type F    | CR102M01/01 |
| Crownstone Guidestone Kit        | Guidestone kit                                     | CE            | 240V      | CR201M01/01 |

The vendor product number exists of [CRfppMmm/vv] with:

    f - family
    pp - product
    mm - market
    vv - variant

The market can be updated as soon as we have products that are for example certified for more markets. We definitely
want to have updated packaging for those markets. However, we do not need to change the product for these markets.
Hence the product number is slightly more general (see below).

# Product number (label on product itself)

The product number should not specify the market. 

| Name                             | Description                                        | Certification | Variant   | VPN         |
| --                               | --                                                 | --            | --        | --          |
| Crownstone Built-in Kit Zero     | Professional kit with three built-in Crownstones   | CE            | 240V      | CR101MXX/01 |
| Crownstone Built-in Kit One      | Professional kit with three built-in Crownstones   | CE & UL       | 110/240V  | CR101MXX/02 |
| Crownstone Plugs Kit             | Ready-to-go kit with two Crownstone plugs          | CE            | Type F    | CR102MXX/01 |
| Crownstone Guidestone Kit        | Guidestone kit                                     | CE            | 240V      | CR201MXX/01 |

For questions, contact [Crownstone](https://crownstone.rocks/team/).

# EAN

| Name                                              | VPN         | EAN           |
| --                                                | --          | --            |
| Crownstone Built-in Zero - Minimal Kit (3 units)  | CR101M01/01 | 7091047158574 |
| Crownstone Plugs - Ready to Go Kit                | CR102M01/01 | 7091048321175 |
| Crownstone Guidestone - Kit (10 units)            | CR201M01/01 | 7091047327949 |
| Crownstone Built-in One - Starter Kit (10 units)  | CR101M01/02 | 8719326566085 |
| Crownstone Built-in One - Extension Kit (1 unit)  | CR101M01/02 | 8719326566085 |
