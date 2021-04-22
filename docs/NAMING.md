# Naming Convention

Naming depends on the purpose, in this document we explain the naming for:

* manufacturing
* packaging
* distribution
* software
* shops

# Manufacturing

A name is a concatenation of multiple fields:

    broad product line | product line | product | part | version i.e. 00 000000 00 X_Name 0.0.0

This naming you can find back in communication to the factories that produce Crownstone products or in inventory documents.

## Product families

Two digits.

    01: Crownstone 'family'
    02: Third-party 'family'

## Product lines within each family

Six digits in binary format. This means that parts can be reused over multiple products.

### Crownstone family

The binary product representation is as follows.

    000001 Development Board     01
    000010 Plug                  02
    000100 Built-in              04
    001000 (Own) Guidestone      08

You see that the Crownstone family has multiple products. To indicate that we can use a particular physical part for e.g. both plugs and guidestones we can use entries in our tables, such as:

    001010 

For example the socket part of the plastic can be reused for both products. Or we might have an electronic component - say, a relays - that can be reused for both the plugs as well as the built-in Crownstones.

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
      Plug_cap_part (the insert of the plug part containing the actual electrical contact materials, pins, ground plate etc.)
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

You see that there are plastic `P_` and metal `M_` parts. You can also see that the bottom four can be used for both the plugs as well as the guidestones `001010`. If we have 10.000 `P_Ring` parts we can create a total of 10.000 plugs/guidestones.

# Packaging

## Vendor product number

The product lines can also be used directly for a vendor product number (VPN). This number is communicated with
distributors. It changes for each type of commercially available product. 

| Name                         | Description                                        | Certification | Variant   | VPN         |
| --                           | --                                                 | --            | --        | --          |
| Crownstone Built-in Zero     | Built-in Crownstone Generation Zero                | CE            | 240V      | CR101M01/01 |
| Crownstone Built-in One      | Built-in Crownstone Generation One                 | CE & FCC      | 110/240V  | CR101M02/02 |
| Crownstone Plug              | Crownstone Plug European Type F                    | CE            | Type F    | CR102M01/01 |
| Crownstone Guidestone        | Guidestone                                         | CE            | 240V      | CR201M01/01 |

The vendor product number exists of `CRfppMmm/vv` with:

    f - family
    pp - product
    mm - market
    vv - variant

Again. The product `pp` number is not the same as the binary product presentation used in manufacturing, it is only bumped when a new product enters the market. If the product is in the same category only the `vv` variant is updated (e.g. from the Crownstone Built-in Zero to the Crownstone Built-in One). This VPN contains market information (which means it is certified for that market). This can be found on the packaging. On the product itself this information is not there, it is shortened, see next section.

# Distribution

## Product number (label on product itself, shortened VPN)

The product number should not specify the market. You will see the following abbreviated VPN on the product itself:

| Name                         | Description                                        | Certification | Variant   | VPN      |
| --                           | --                                                 | --            | --        | --       |
| Crownstone Built-in Zero     | Built-in Crownstone Generation Zero                | CE            | 240V      | CR101/01 |
| Crownstone Built-in One      | Built-in Crownstone Generation One                 | CE & FCC      | 110/240V  | CR101/02 |
| Crownstone Plug              | Crownstone Plug European Type F                    | CE            | Type F    | CR102/01 |
| Crownstone Guidestone        | Guidestone                                         | CE            | 240V      | CR201/01 |

# Software

## Registers

When the hardware is programmed the factory image contains information on the product as well. The following information is stored separately from the main firmware (which is identical for every device). Note that these are stored in registers and use fewer bits than the notations on the packaging, in the code, or in the documentation.

| Setting                       | Description                 | Example             |
| --                            | --                          | --                  |
| MBR_SETTINGS                  | MBR settings page (start)   | 0x0007E000          |
| UICR_BOOTLOADER_ADDRESS       | Bootloader address (start)  | 0x00076000          |
| HARDWARE_BOARD                | PCB version                 | ACR01B10D           |
| PRODUCT_FAMILY                | Family                      | 0x01                |
| PRODUCT_MARKET                | Market (European / US)      | 0x01                |
| PRODUCT_TYPE                  | Type (different from above) | 0x03                |
| PRODUCT_MAJOR                 | Major version               | 0x00                |
| PRODUCT_MINOR                 | Minor version               | 0x05                |
| PRODUCT_PATCH                 | Patch version               | 0x00                |
| PRODUCTION_YEAR               | Production year (2 digits)  | 19                  |
| PRODUCTION_WEEK               | Production week (2 digits)  | 50                  |
| PRODUCT_HOUSING_ID            | Housing id                  | 0x01                |

The above settings are not directly stored in separate registers, but concatenated to preserve space.

| UICR register    | Description             | Example             |
| --               | --                      | --                  |
| 0x10001084       | Hardware board          | 0x3EC               |
| 0x10001088       | Family/market/type      | 0xFF010103          |
| 0x1000108c       | Major/minor/patch       | 0xFF000500          |
| 0x10001090       | Production date/housing | 0xFF191201          |

The hardware board `0x3EC` corresponds to `1004` in decimal notation. In the [cs_Boards.h](https://github.com/crownstone/bluenet/blob/master/source/include/cfg/cs_Boards.h) file this is board `ACR01B1E`. Likewise `ACR01B10D` has number `1008` is `0x3F0`.

The major/minor versions are currently generated from the hardware board id, see the [cs_HardwareVersions.h](https://github.com/crownstone/bluenet/blob/master/source/include/cfg/cs_HardwareVersions.h) file. A patch is a PCB patch which can be taken into account by the firmware to e.g. delay enabling the dimmer at boot. A minor update is done for any PCB alteration that does not change the pin layout. A major update means significant hardware changes, under which pin layout changes.

The family / market / type triplet is different from the notation at the top of this document. 

    Development Board     01
    Plug                  02
    Built-in              03
    Guidestone            04
    Dongle                05
    Built-in One          06
    Plug One              07
    Hub                   08

If values are not written they are still `FF`.

Why for example the `PRODUCT_HOUSING` is important to be known to the firmware, is because a different form factor of the product housing might influence the antenna. It might very well be that additional metal requires us to broadcast slightly stronger for example or it might require us to use different parameters for the current measurements or modules that depend on the current measurements. The id for `PRODUCT_HOUSING` starts with `0x01` for each product line and is bumped for each mechanical alteration.

# Shops

## EAN

| Name                                              | VPN         | EAN           |
| --                                                | --          | --            |
| Crownstone Built-in Zero - Minimal Kit (3 units)  | CR101M01/01 | 7091047158574 |
| Crownstone Plugs - Ready to Go Kit                | CR102M01/01 | 7091048321175 |
| Crownstone Guidestone - Kit (10 units)            | CR201M01/01 | 7091047327949 |
| Crownstone Built-in One - Starter Kit (10 units)  | CR101M01/02 | 8719326566085 |
| Crownstone Built-in One - Extension Kit (1 unit)  | CR101M01/02 | 8719326566092 |
| Crownstone Hub                                    | CR103M01/01 | 8719326566078 |

For questions, contact [Crownstone](https://crownstone.rocks/team/).
