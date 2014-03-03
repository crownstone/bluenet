
# NRF51822 GCC Makefile

# ENVIRONMENT VARIABLES ############################################################################

# Please either set these variables in your environment or uncomment and edit here:

# PREFIX for gcc compiler (bin/arm-none-eabi-gcc is here)  Use a recent version, like >= 4.7.2
#ARM_DIR=/usr/local/src/teensy/gcc-arm-none-eabi-4_7-2013q2

# set on path

#SOFTDEVICE_ROOT=
#NORDIC_ROOT=

# PATH to jlink flasher binaries (JLinkExe is here)
#JLINK_DIR=/usr/local/src/jlink

-include local.mk

# CONFIGURABLE #####################################################################################

# whether to use the soft device ("1") or not ("0").  BLE examples require it.
NRF51_USE_SOFTDEVICE=1

# Available optimization levels are -O0, -O1, -O2, and -Os (for "size").  -O0 is useful for debugging.
OPTIMIZE=-Os -fomit-frame-pointer -ffast-math -fno-permissive
#OPTIMIZE=-O0

# version of NRF51822 softdevice. 
SOFTDEVICE_VERS=5.2.1
NORDIC_SDK_VERS=4_4_1_31827
#NORDIC_SDK_VERS=4_4_0_30888

SOFTDEVICE=s110_nrf51822_${SOFTDEVICE_VERS}

# directory containing NRF51822 softdevice (.hex is here.)
SOFTDEVICE_DIR=${SOFTDEVICE_ROOT}
#/${SOFTDEVICE}

# directory containing NRF51822 SDK (contains Source, Include)
#NRF51822_DIR=../nrf51_sdk_v${NORDIC_SDK_VERS}/nrf51822
#NRF51822_DIR=../nordic/nrf51822
NRF51822_DIR=${NORDIC_ROOT}

LIB_DIRS= usvg epaper Adafruit_GFX midi

BOARD=E_STICKY_v1
#BOARD=EPAPER_DEMO_BOARD

### SDK Related variables
###
SDK 		 = lib/nrf51sdk/Nordic/nrf51822/
SDK_INCLUDE	 = $(SDK)Include/
SDK_SRC		 = $(SDK)Source/
SDK_TEMPLATE	 = $(SDK_SRC)templates/gcc/
SDK_APP_COMMON 	 = $(SDK_SRC)app_common/
SDK_EXT_SENSORS	 = $(SDK_SRC)ext_sensors/
SDK_BLE		 = ble/
SDK_BLE		+= ble/ble_services/

# END CONFIGURABLE #################################################################################


.DEFAULT_GOAL = all
.SECONDARY: # don't delete intermediates
	#nothing

DYLD_LIBRARY_PATH:=${JLINK_DIR}

export DYLD_LIBRARY_PATH

# path location for the arm-none-eabi compiler
COMPILERPATH = ${ARM_DIR}/bin

# names for the compiler programs
CC = $(COMPILERPATH)/arm-none-eabi-gcc
CXX = $(COMPILERPATH)/arm-none-eabi-g++
AS = $(COMPILERPATH)/arm-none-eabi-as
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy
SIZE = $(COMPILERPATH)/arm-none-eabi-size
GDB=$(COMPILERPATH)/arm-none-eabi-gdb
NM=$(COMPILERPATH)/arm-none-eabi-nm

#for some reason it's called JLink on windows and JLinkExe on mac.
ifeq ($(OS),Windows_NT)
JLINK_EXE=${JLINK_DIR}/JLink
else
JLINK_EXE=${JLINK_DIR}/JLinkExe
endif
JLINK_GDB=${JLINK_DIR}/JLinkGDBServer
JLINK_GDB_ARGS= -if SWD -device nRF51822 -speed 400 -log jlinkgdbserver.log

SOFTDEVICE_INCLUDE=${SOFTDEVICE_DIR}/${SOFTDEVICE}_api/include

NRF51822_SOURCE_BASE=${NRF51822_DIR}/Source
NRF51822_SOURCE_SUBDIRS=app_common ${SDK_BLE} nrf_assert nrf_delay
NRF51822_SOURCES = $(addprefix ${NRF51822_SOURCE_BASE}/%,${NRF51822_SOURCE_SUBDIRS})
NRF51822_INCLUDE_BASE=${NRF51822_DIR}/Include
NRF51822_INCLUDE_SUBDIRS=app_common ${SDK_BLE} ble/softdevice gcc
NRF51822_INCLUDES= -I${NRF51822_INCLUDE_BASE} $(patsubst %,-I${NRF51822_INCLUDE_BASE}/%,${NRF51822_INCLUDE_SUBDIRS})
INCLUDES = -I- -I../include -I../assets -I. -I${SOFTDEVICE_INCLUDE} ${NRF51822_INCLUDES}  $(foreach dir,${LIB_DIRS}, -I../libs/${dir})

DEFINES=-DNRF51 -DEPD_ENABLE_EXTRA_SRAM -DARDUINO=100 -D${BOARD} -DNRF51_USE_SOFTDEVICE=${NRF51_USE_SOFTDEVICE} -DUSE_RENDER_CONTEXT -DSYSCALLS -DUSING_FUNC
CFLAGS = -std=gnu99
CXXFLAGS = -std=c++11 -fno-exceptions
CPPFLAGS = -Wall -g3 ${OPTIMIZE} -mcpu=cortex-m0 -mthumb -nostdlib -ffunction-sections -fdata-sections -MMD ${DEFINES} ${INCLUDES}
LDFLAGS = ${OPTIMIZE} -g3 -mcpu=cortex-m0 -mthumb -TnRF51822-$(if $(filter 1,$(NRF51_USE_SOFTDEVICE)),softdevice,bare).ld -Wl,--gc-sections

# Use size-optimized libraries instead
#LIBS= -lsupc++ -lm -lc -lstdc++
LIBS = -lsupc++_s -lm -lc_s -lstdc++_s

C_FILES := $(foreach dir,${LIB_DIRS}, $(wildcard ../libs/$(dir)/*.c)) $(wildcard *.c)

# this is a total hack to get rid of some files from the nordic SDK (hci_* and uart stuff) that don't place nicely together.
NRF_C_FILES := $(foreach dir,${NRF51822_SOURCE_SUBDIRS}, $(filter-out ${NRF51822_SOURCE_BASE}/app_common/app_uart_fifo.c,$(filter-out ${NRF51822_SOURCE_BASE}/$(dir)/hci_%,$(wildcard ${NRF51822_SOURCE_BASE}/$(dir)/*.c))))
CPP_FILES := $(wildcard *.cpp) $(foreach dir,${LIB_DIRS}, $(wildcard ../libs/$(dir)/*.cpp))
ALL_OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o) $(NRF_C_FILES:.c=.o)
ALL_DEPS := $(CPP_FILES:.cpp=.d) $(C_FILES:.c=.d) $(NRF_C_FILES:.c=.d)
OBJS := $(filter-out main%,$(ALL_OBJS))  # note: overridden below for some targets.
MAINS := $(filter main%,$(ALL_OBJS))



NRF_UICR_BIN=${SOFTDEVICE_DIR}_softdevice_uicr.bin
NRF_MAIN_BIN=${SOFTDEVICE_DIR}_softdevice_mainpart.bin

${NRF_UICR_BIN}:
	${OBJCOPY} -Iihex -Obinary --only-section .sec3 ${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex ${NRF_UICR_BIN}
${NRF_MAIN_BIN}:
	${OBJCOPY} -Iihex -Obinary --remove-section .sec3 ${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex ${NRF_MAIN_BIN}

#all: main_blink.bin reset debug
#all: last main_hrs.bin debug

all: main_ble.elf main_ble.bin debug

# throws an error
#all: last main_ble.bin debug


# Here we redefine the OBJS needed for specific executables.  We do this because there are certain Nordic
# .c files that define things like IRQ handlers which conflict either with other Nordic .c files, or
# with things that we want to define.
# This uses strong magic: http://stackoverflow.com/questions/9311743/make-using-target-specific-variables-in-prerequisites

.SECONDEXPANSION:

main_blink.elf: OBJS := Arduino.o Print.o Stream.o error.o emergency.o nRF51822.o syscalls.o

main_display.elf: OBJS := Arduino.o BluetoothLE.o Debug.o NRF51_SPI.o Print.o Stream.o error.o ../libs/usvg/Display.o \
	../libs/usvg/DisplayObject.o ../libs/usvg/Font.o ../libs/usvg/Pool.o ../libs/usvg/RLEImage.o ../libs/usvg/random.o \
	../libs/epaper/EPD.o ../libs/epaper/EPD_Display.o ../libs/epaper/EPD_GFX.o ../libs/epaper/S5813A.o \
	../libs/Adafruit_GFX/Adafruit_GFX.o emergency.o nRF51822.o syscalls.o ${NRF51822_SOURCE_BASE}/ble/ble_advdata.o \
	${NRF51822_SOURCE_BASE}/ble/ble_debug_assert_handler.o ${NRF51822_SOURCE_BASE}/nrf_assert/nrf_assert.o

main_gfxtest.elf: OBJS := Arduino.o Debug.o NRF51_SPI.o Print.o Stream.o error.o ../libs/usvg/Display.o \
	../libs/usvg/DisplayObject.o ../libs/usvg/Font.o ../libs/usvg/Pool.o ../libs/usvg/RLEImage.o ../libs/usvg/HuffmanImage.o ../libs/usvg/random.o \
	../libs/epaper/EPD.o ../libs/epaper/EPD_Display.o ../libs/epaper/EPD_GFX.o ../libs/epaper/S5813A.o \
	../libs/Adafruit_GFX/Adafruit_GFX.o emergency.o nRF51822.o syscalls.o

main_hufftest.elf: OBJS := Arduino.o Debug.o NRF51_SPI.o Print.o Stream.o error.o ../libs/usvg/Display.o \
	../libs/usvg/DisplayObject.o ../libs/usvg/Font.o ../libs/usvg/Pool.o ../libs/usvg/RLEImage.o ../libs/usvg/HuffmanImage.o ../libs/usvg/random.o \
	../libs/epaper/EPD.o ../libs/epaper/EPD_Display.o ../libs/epaper/EPD_GFX.o ../libs/epaper/S5813A.o \
	../libs/Adafruit_GFX/Adafruit_GFX.o emergency.o nRF51822.o syscalls.o

main_displaylisttest.elf: OBJS := Arduino.o Debug.o NRF51_SPI.o Print.o Stream.o error.o ../libs/usvg/Display.o \
        ../libs/usvg/DisplayObject.o ../libs/usvg/Font.o ../libs/usvg/Pool.o ../libs/usvg/RLEImage.o ../libs/usvg/HuffmanImage.o ../libs/usvg/random.o \
        ../libs/epaper/EPD.o ../libs/epaper/EPD_Display.o ../libs/epaper/EPD_GFX.o ../libs/epaper/S5813A.o \
        ../libs/Adafruit_GFX/Adafruit_GFX.o emergency.o nRF51822.o syscalls.o

# Thows an exception.
# Catchpoint 3 (exception thrown), 0x0001763c in __cxa_throw ()
main_ble.elf: OBJS := Arduino.o BluetoothLE.o Debug.o NRF51_SPI.o Print.o Stream.o error.o ../libs/usvg/Pool.o \
    emergency.o nRF51822.o syscalls.o ${NRF51822_SOURCE_BASE}/ble/ble_advdata.o \
	${NRF51822_SOURCE_BASE}/ble/ble_debug_assert_handler.o ${NRF51822_SOURCE_BASE}/nrf_assert/nrf_assert.o

main_hrs.elf: OBJS := nRF51822.o syscalls.o \
	${NRF51822_SOURCE_BASE}/app_common/app_button.o \
	${NRF51822_SOURCE_BASE}/app_common/app_fifo.o ${NRF51822_SOURCE_BASE}/app_common/app_gpiote.o \
	${NRF51822_SOURCE_BASE}/app_common/app_scheduler.o ${NRF51822_SOURCE_BASE}/app_common/app_timer.o \
	${NRF51822_SOURCE_BASE}/ble/ble_advdata.o \
	${NRF51822_SOURCE_BASE}/ble/ble_bondmngr.o ${NRF51822_SOURCE_BASE}/ble/ble_conn_params.o \
	${NRF51822_SOURCE_BASE}/ble/ble_debug_assert_handler.o \
	${NRF51822_SOURCE_BASE}/ble/ble_error_log.o ${NRF51822_SOURCE_BASE}/ble/ble_flash.o \
	${NRF51822_SOURCE_BASE}/ble/ble_racp.o ${NRF51822_SOURCE_BASE}/ble/ble_radio_notification.o \
	${NRF51822_SOURCE_BASE}/ble/ble_sensorsim.o ${NRF51822_SOURCE_BASE}/ble/ble_stack_handler.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_dis.o ${NRF51822_SOURCE_BASE}/ble/ble_services/ble_bas.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_hrs.o \
	${NRF51822_SOURCE_BASE}/ble/ble_srv_common.o ${NRF51822_SOURCE_BASE}/nrf_assert/nrf_assert.o

main_prox.elf: OBJS := nRF51822.o syscalls.o \
	${NRF51822_SOURCE_BASE}/app_common/app_button.o \
	${NRF51822_SOURCE_BASE}/app_common/app_fifo.o ${NRF51822_SOURCE_BASE}/app_common/app_gpiote.o \
	${NRF51822_SOURCE_BASE}/app_common/app_scheduler.o ${NRF51822_SOURCE_BASE}/app_common/app_timer.o \
	${NRF51822_SOURCE_BASE}/ble/ble_advdata.o \
	${NRF51822_SOURCE_BASE}/ble/ble_bondmngr.o ${NRF51822_SOURCE_BASE}/ble/ble_conn_params.o \
	${NRF51822_SOURCE_BASE}/ble/ble_debug_assert_handler.o \
	${NRF51822_SOURCE_BASE}/ble/ble_error_log.o ${NRF51822_SOURCE_BASE}/ble/ble_flash.o \
	${NRF51822_SOURCE_BASE}/ble/ble_racp.o ${NRF51822_SOURCE_BASE}/ble/ble_radio_notification.o \
	${NRF51822_SOURCE_BASE}/ble/ble_sensorsim.o ${NRF51822_SOURCE_BASE}/ble/ble_stack_handler.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_dis.o ${NRF51822_SOURCE_BASE}/ble/ble_services/ble_bas.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_ias_c.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_ias.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_tps.o \
	${NRF51822_SOURCE_BASE}/ble/ble_services/ble_lls.o \
	${NRF51822_SOURCE_BASE}/ble/ble_srv_common.o ${NRF51822_SOURCE_BASE}/nrf_assert/nrf_assert.o


# end magic.


%.elf: %.o $$(OBJS) nRF51822-$(if $(NRF51_USE_SOFTDEVICE),softdevice,bare).ld
	echo "$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(@:.elf=.o) $(OBJS) $(LIBS)" > compile.txt
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(@:.elf=.o) $(OBJS) $(LIBS)
	@ echo $@ > .last


%.bin: %.elf
	$(SIZE) $<
	$(OBJCOPY) -j .text -j .data -O binary $< $@
	@echo $@ > .lastbin
	

last:
	make -C . `cat .lastbin`

clean:
	-rm -f $(ALL_OBJS) $(ALL_DEPS) *.d *.elf *.hex *.bin *.log *.map $(NRF_UICR_BIN) $(NRF_MAIN_BIN)
#$(MAKE) -C ../assets/fonts clean

reset: clean ${NRF_UICR_BIN} ${NRF_MAIN_BIN}
	@ERR=$(killall -9 -q JLinkGDBServer)
	sed "s|@SOFTDEVICE_DIR@|${SOFTDEVICE_DIR}|" ../scripts/reset.script > reset.script 
	echo "$(JLINK_EXE) -Device nrf51822 -If SWD reset.script" > cmd.sh
	. ./cmd.sh

recover:
	echo "$(JLINK_EXE) -Device nrf51822 -If SWD ../scripts/recover.script" > cmd.sh
	. ./cmd.sh

power:
	-killall -9 -q JLinkGDBServer
	cat ../scripts/power.script | "$(JLINK_EXE)" -Device nrf51822 -If SWD

upload:
	@echo "killall -9 -q JLinkGDBServer" > cmd.sh
	@echo "exit 0" >> cmd.sh
	@. ./cmd.sh
	sed "s/@BIN@/"`cat .lastbin`"/" ../scripts/upload.script > upload.script
	@echo "$(JLINK_EXE) -Device nrf51822 -If SWD upload.script" > cmd.sh
	@echo "exit 0" >> cmd.sh
	@. ./cmd.sh

wipe: clean
	-killall -9 -q JLinkGDBServer
	-"$(JLINK_EXE)" -Device nrf51822 -If SWD ../scripts/wipe.script

debug:
	@echo "killall -9 -q JLinkGDBServer" > cmd.sh
	@echo "exit 0" >> cmd.sh
	@. ./cmd.sh
	echo "${JLINK_GDB}" ${JLINK_GDB_ARGS} 
	"${JLINK_GDB}" ${JLINK_GDB_ARGS} > JLinkGdb.log &
	echo -"${GDB}" -x ../scripts/gdbinit `cat .last`
	-"${GDB}" -x ../scripts/gdbinit `cat .last`
	-killall -9 -q JLinkGDBServer

size:
	${NM} --size-sort -S -l -C -A `cat .last`

softdevice: reset

fonts:
	$(MAKE) -C ../assets/fonts

main_display.o: fonts

# compiler generated dependency info
#-include $(OBJS:.o=.d)
