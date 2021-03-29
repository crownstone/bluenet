# Adding a new board

## Information required

The pin layout can be found online for e.g. the [nRF52832](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fpin.html&cp=4_2_0_3&anchor=pin_assign) and the [nRF52833](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52833%2Fpin.html).

The nRF52833 has mappings like:

| Pin   | Name          | Crownstone Description | Pin nr |
| ----- | ------------- | ---------------------- | ------ |
| A8    | P0.31 (AIN7)  | V1                     | 31     |
| A10   | P0.29 (AIN5)  | V2                     | 29     |
| A12   | P0.02 (AIN0)  | I1                     |  2     |
| B13   | P0.03 (AIN1)  | I2                     |  3     |
| AD8   | P0.13         | reset (relay)          | 13     |
| AD10  | P0.15         | set (relay)            | 15     |
| J1    | P0.04 (AIN2)  | NTC <needs patch>      |  4     |
| A14   | P0.19         | floating               | 19     |
| N1    | P0.08         | PWM                    |  8     |
| K2    | P0.05 (AIN3)  | Reference voltage      |  5     |
| AD18  | P0.22         | RX                     | 22     |
| AD16  | P0.20         | TX                     | 20     |
| AD20  | P0.24         | GPIO1                  | 24     |
| AD22  | P1.00         | GPIO2                  | 32     |
| W24   | P1.02         | GPIO3                  | 34     |
| U24   | P1.04         | GPIO4                  | 36     |
| AC24  | SWDIO         | SWDIO                  |  X     |
| AA24  | SWCLK         | SWDCLK                 |  X     |

Here the pin and name can be found at Nordic. To understand the functionality of each pin, you will have to look at
the PCB itself, read the schematics files (if you have them), or contact Crownstone. There can also be additional
pins, e.g. to enable the dimming functionality, or there might be some LEDs.

The mapping for pins at port 1 is ` (((port) << 5) | ((pin) & 0x1F))`. For example, pin P1.00 will map to 32.

For example on the ACR01B11A board there is an [UCC5310](http://www.ti.com/lit/ds/symlink/ucc5310.pdf) gate driver. In
10.4 Device Functional Modes there is a table. Here the first two columns are reverted given that we can only control
IN-.

| IN-   | IN+     | OUT  |
| ----- | ------- | ---- |
| high  | X       | low  |
| low   | high    | high |

Hence, it is inverting (OUT is almost directly coupled to the gates of the IGBTs).

## Files to be adjusted

in order to add a new board you need to alter multiple files. These are:
* [cs_Boards.h](../include/cfg/cs_Boards.h)
* [cs_Boards.c](../src/cfg/cs_Boards.c)
* [cs_HardwareVersions.h](../include/cfg/cs_HardwareVersions.h)
* [CMakeBuild.config.default](../conf/cmake/CMakeBuild.config.default)

### cs_Boards.h

Here you just need to add the new board to the list of defined boards and give tie the new board to a decimal value that can be described by a 32bit variable.
The name you define here will be used in the other files to refer to your new board.

### cs_Boards.c

Here you will need to define the pinout of your new board. You can copy the template at the top of the file and fill in the appropriate pin numbers and options. Change the function name to reflect your new board, i.e. `void asYOURNEWBOARD(...`

After adding your new board as a function, make sure it can be called by adding it to the switch case statement at the bottom of the cs_Boards.c file. The case your will expect for your new board is the name you supplied in cs.Boards.h.

If your new board has a pinout that is the same as a previous board, you don't need to add a new void function for it. You will need to refer to the correct option in the switch case option for you own board name however.

### cs_HardwareVersion.h

Add your new board version to this file by following the guide and examples.

### CMakeBuild.config.default

Add you new board in the list of commented boards. This will ensure that someone will be able to find and implement your new board easily in the future.
