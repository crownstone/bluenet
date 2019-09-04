## Adding a new board

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
