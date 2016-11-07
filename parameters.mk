# Parameters for the esp-open-rtos make process
#
# You can edit this file to change parameters, but a better option is
# to create a local.mk file and add overrides there. The local.mk file
# can be in the root directory, or the program directory alongside the
# Makefile, or both.
#
-include $(ROOT)local.mk
-include local.mk

# Flash size in megabits
# Valid values are same as for esptool.py - 2,4,8,16,32
ifeq ($(PLATFORM),esp8266)
FLASH_SIZE ?= 32
endif

ifeq ($(PLATFORM),esp32)
FLASH_SIZE ?= 2MB
endif

# Flash mode, valid values are same as for esptool.py - qio,qout,dio.dout
FLASH_MODE ?= dio

# Flash speed in MHz, valid values are same as for esptool.py - 80, 40, 26, 20
FLASH_SPEED ?= 40

# Output directories to store intermediate compiled files
# relative to the program directory
BUILD_DIR ?= $(ROOT)main/platform/$(PLATFORM)/build/
FIRMWARE_DIR ?= $(ROOT)main/platform/$(PLATFORM)/firmware/

# esptool.py from https://github.com/themadinventor/esptool
ifeq ($(PLATFORM),esp8266)
ESPTOOL ?= esptool.py
endif

ifeq ($(PLATFORM),esp32)
ESPTOOL ?= $(ROOT)main/platform/esp32/esp-idf/components/esptool_py/esptool/esptool.py
endif

ifeq ($(PLATFORM),pic32mz)
ESPTOOL ?= esptool.py
endif

# serial port settings for esptool.py
ESPPORT ?= /dev/ttyUSB0

ifeq ($(PLATFORM),esp8266)
ESPBAUD ?= 115200
endif

ifeq ($(PLATFORM),esp32)
ESPBAUD ?= 921600
endif

# firmware tool arguments
ifeq ($(PLATFORM),esp8266)
ESPTOOL_ARGS=-fs $(FLASH_SIZE)m -fm $(FLASH_MODE) -ff $(FLASH_SPEED)m
endif

ifeq ($(PLATFORM),esp32)
ESPTOOL_ARGS=-fs $(FLASH_SIZE) -fm $(FLASH_MODE) -ff $(FLASH_SPEED)m
endif


# set this to 0 if you don't need floating point support in printf/scanf
# this will save approx 14.5KB flash space and 448 bytes of statically allocated
# data RAM
#
# NB: Setting the value to 0 requires a recent esptool.py (Feb 2016 / commit ebf02c9)
PRINTF_SCANF_FLOAT_SUPPORT ?= 1

FLAVOR ?= release

# Compiler names, etc. assume gdb
ifeq ($(PLATFORM),esp8266)
CROSS ?= xtensa-lx106-elf-
endif

ifeq ($(PLATFORM),esp32)
CROSS ?= xtensa-esp32-elf-
endif

ifeq ($(PLATFORM),pic32mz)
CROSS ?= mips-elf-
endif

# Path to the filteroutput.py tool
FILTEROUTPUT ?= $(ROOT)/utils/filteroutput.py

AR = $(CROSS)ar
CC = $(CROSS)gcc
CPP = $(CROSS)cpp
LD = $(CROSS)gcc
NM = $(CROSS)nm
C++ = $(CROSS)g++
SIZE = $(CROSS)size  
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

# Source components to compile and link. Each of these are subdirectories
# of the root, with a 'component.mk' file.

ifeq ($(PLATFORM),esp8266)
COMPONENTS ?= $(EXTRA_COMPONENTS) FreeRTOS main main/platform/$(PLATFORM)/lwip main/platform/$(PLATFORM)/core sys pthread Lua main/platform/$(PLATFORM)/open_esplibs
endif

ifeq ($(PLATFORM),esp32)
COMPONENTS ?= main pthread Lua sys
endif

ifeq ($(PLATFORM),pic32mz)
COMPONENTS ?= $(EXTRA_COMPONENTS) FreeRTOS sys sys/drivers/lmic pthread Lua main main/platform/$(PLATFORM)/quad
endif


# binary esp-iot-rtos SDK libraries to link. These are pre-processed prior to linking.
ifeq ($(PLATFORM),esp8266)
SDK_LIBS = main net80211 phy pp wpa
endif

# open source libraries linked in
ifeq ($(PLATFORM),esp8266)
LIBS ?= hal gcc c m
endif

ifeq ($(PLATFORM),esp32)
LIBS ?= esp32 driver log spi_flash nvs_flash core rtc c hal gcc m c_rom phy freertos newlib vfs net80211 pp wpa lwip
endif

ifeq ($(PLATFORM),pic32mz)
LIBS ?= c m
endif

# set to 0 if you want to use the toolchain libc instead of esp-open-rtos newlib
OWN_LIBC ?= 1

# Note: you will need a recent esp
ENTRY_SYMBOL ?= call_user_start

# Set this to zero if you don't want individual function & data sections
# (some code may be slightly slower, linking will be slighty slower,
# but compiled code size will come down a small amount.)
SPLIT_SECTIONS ?= 1

# Set this to 1 to have all compiler warnings treated as errors (and stop the
# build).  This is recommended whenever you are working on code which will be
# submitted back to the main project, as all submitted code will be expected to
# compile without warnings to be accepted.
WARNINGS_AS_ERRORS ?= 0

# Common flags for both C & C++_
C_CXX_FLAGS ?= -Wall -Wl,-EL -nostdlib $(EXTRA_C_CXX_FLAGS)
# Flags for C only
CFLAGS		?= $(C_CXX_FLAGS) -std=gnu99 $(EXTRA_CFLAGS) -D__XMK__ -fno-builtin -DKERNEL
# Flags for C++ only
CXXFLAGS	?= $(C_CXX_FLAGS) -fno-exceptions -fno-rtti $(EXTRA_CXXFLAGS)

# these aren't all technically preprocesor args, but used by all 3 of C, C++, assembler

ifeq ($(PLATFORM),esp8266)
CPPFLAGS += -mlongcalls -mtext-section-literals
endif

ifeq ($(PLATFORM),esp32)
CFLAGS += -DESP_PLATFORM -nostdinc -U_POSIX_THREADS -mlongcalls -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -nostdlib -MMD -MP
endif

ifeq ($(PLATFORM),pic32mz)
CFLAGS += -mips32r2 -EL -mhard-float -fno-short-double -mfp64 -G 0
endif

include $(ROOT)main/platform/$(PLATFORM)/config.mk
ifeq ($(PLATFORM),esp8266)
EXTRA_LDFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free
endif

ifeq ($(PLATFORM),pic32mz)
endif

ifeq ($(PLATFORM),esp8266)
LDFLAGS	= -nostdlib -L$(BUILD_DIR)sdklib -u $(ENTRY_SYMBOL) -Wl,--no-check-sections -Wl,-Map=$(BUILD_DIR)$(PROGRAM).map $(EXTRA_LDFLAGS)
endif

ifeq ($(PLATFORM),esp32)
LDFLAGS	= -nostdlib -L$(ROOT)lib -L$(ROOT)lib/platform/esp32 -u call_user_start_cpu0 -Wl,--gc-sections,-u,main  -Wl,-static -Wl,-Map=$(BUILD_DIR)$(PROGRAM).map $(EXTRA_LDFLAGS)
endif

ifeq ($(PLATFORM),pic32mz)
LDFLAGS	= -mips32r2 -EL -nostdlib -nostartfiles -Wl,--oformat=elf32-littlemips -o LuaOS_V1.elf -Wl,-z,max-page-size=4096,-Os -Wl,-Map=LuaOS_V1.map -Wl,--gc-sections,-u,main  $(EXTRA_LDFLAGS)
endif

CFLAGS += -DUSE_CUSTOM_HEAP=0

ifeq ($(PLATFORM),esp8266)
LINKER_SCRIPTS += $(ROOT)main/platform/$(PLATFORM)/ld/program.ld $(ROOT)main/platform/$(PLATFORM)/ld/rom.ld
endif

ifeq ($(PLATFORM),esp32)
LINKER_SCRIPTS += $(ROOT)main/platform/$(PLATFORM)/ld/esp32.ld $(ROOT)main/platform/$(PLATFORM)/ld/esp32.rom.ld
LINKER_SCRIPTS += $(ROOT)main/platform/$(PLATFORM)/ld/esp32.common.ld $(ROOT)main/platform/$(PLATFORM)/ld/esp32.peripherals.ld
endif

ifeq ($(PLATFORM),pic32mz)
LINKER_SCRIPTS += $(ROOT)main/platform/$(PLATFORM)/ld/program.ld
endif

ifeq ($(WARNINGS_AS_ERRORS),1)
    C_CXX_FLAGS += -Werror
endif

ifeq ($(SPLIT_SECTIONS),1)
  C_CXX_FLAGS += -ffunction-sections -fdata-sections
  LDFLAGS += -Wl,-gc-sections
endif

ifeq ($(FLAVOR),debug)
    C_CXX_FLAGS += -g -O0
    LDFLAGS += -g -O0
else ifeq ($(FLAVOR),sdklike)
    # These are flags intended to produce object code as similar as possible to
    # the output of the compiler used to build the SDK libs (for comparison of
    # disassemblies when coding replacement routines).  It is not normally
    # intended to be used otherwise.
    CFLAGS += -O2 -Os -fno-inline -fno-ipa-cp -fno-toplevel-reorder -fno-caller-saves -fconserve-stack
    LDFLAGS += -O2
else
    C_CXX_FLAGS += -g -O2

	ifeq ($(PLATFORM),esp8266)
    LDFLAGS += -g -O2
	endif

	ifeq ($(PLATFORM),esp32)
    LDFLAGS += -Og
	endif
	
	ifeq ($(PLATFORM),pic32mz)
    LDFLAGS += -g -O2
	endif
endif

GITSHORTREV=\"$(shell cd $(ROOT); git rev-parse --short -q HEAD 2> /dev/null)\"
ifeq ($(GITSHORTREV),\"\")
  GITSHORTREV="\"(nogit)\"" # (same length as a short git hash)
endif
CPPFLAGS += -DGITSHORTREV=$(GITSHORTREV)

# rboot firmware binary paths for flashing
RBOOT_BIN = $(ROOT)main/platform/$(PLATFORM)/bootloader/firmware/rboot.bin
RBOOT_PREBUILT_BIN = $(ROOT)main/platform/$(PLATFORM)/bootloader/firmware_prebuilt/rboot.bin
RBOOT_CONF = $(ROOT)main/platform/$(PLATFORM)/bootloader/firmware_prebuilt/blank_config.bin

# if a custom bootloader hasn't been compiled, use the
# prebuilt binary from the source tree
ifeq (,$(wildcard $(RBOOT_BIN)))
RBOOT_BIN=$(RBOOT_PREBUILT_BIN)
endif
