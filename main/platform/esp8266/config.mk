CFLAGS += -DRBOOT_GPIO_ENABLED=0
CFLAGS += -DPLATFORM_ESP8266

#
# LuaOS configuration
# 
CFLAGS += -DCPU_HZ=80000000L                    # CPU frequency in hertz
CFLAGS += -DCORE_TIMER_HZ=CPU_HZ                # CPU core timer frequency
CFLAGS += -D_CLOCKS_PER_SEC_=configTICK_RATE_HZ # Number of interrupt ticks for reach 1 second
CFLAGS += -DBASE_TIMER_TICK_RATE=100            # Generate a FreeRTOS ticks every 100 TIMER1 ticks (T = 1 msec)
CFLAGS += -DUSE_NETWORKING=0                    # Networking is used (1 = yes, 0 = not)
CFLAGS += -DMTX_USE_EVENTS=0                    # Use event groups in mtx implementation (experimental)

CFLAGS += -DluaTaskStack=192*5                  # Stck size assigned to lua thread
CFLAGS += -DtskDEF_PRIORITY=0				    # Default task priority
CFLAGS += -DdefaultThreadStack=192*5

CFLAGS += -DUSE_RTC=0						    # Enable RTC
CFLAGS += -DLED_ACT=0					 	    # GPIO for activity led (0 if not led)
CFLAGS += -DLED_DBG=0 					 	    # GPIO for debug led (0 if not led)

#
# File system configuration
# 
CFLAGS += -DUSE_FAT=0                  # Enable FAT
CFLAGS += -DUSE_SPIFFS=1               # Enable SPIFFS

#
# SPIFFS file system configuration
# 
CFLAGS += -DSPIFFS_ERASE_SIZE=4096     # SPI FLASH sector size (see your flah's datasheet)
CFLAGS += -DSPIFFS_LOG_PAGE_SIZE=256   # SPI FLASH page size (see your flah's datasheet)
CFLAGS += -DSPIFFS_LOG_BLOCK_SIZE=8192 # Logical block size, must be a miltiple of the page size
CFLAGS += -DSPIFFS_BASE_ADDR=0x100000  # SPI FLASH start adress for SPIFFS
CFLAGS += -DSPIFFS_SIZE=0x80000        # SPIFFS size

SPIFFS_ESPTOOL_ARGS = 0x100000 $(BUILD_DIR)spiffs_image.img

#
# Console configuration
#
CFLAGS += -DUSE_CONSOLE=1		       # Enable console
CFLAGS += -DCONSOLE_BR=115200	       # Console baud rate
CFLAGS += -DCONSOLE_UART=1		       # Console UART unit
CFLAGS += -DCONSOLE_SWAP_UART=2	       # Console alternative UART unit (0 = don't use alternative UART)
CFLAGS += -DCONSOLE_BUFFER_LEN=255     # Console buffer length in bytes

#
# LoraWAN driver connfiguration for RN2483
#
CFLAGS += -DUSE_LMIC=0
CFLAGS += -DUSE_RN2483=1

CFLAGS += -DLORA_UART=3				   # RN2483 UART unit
CFLAGS += -DLORA_UART_BR=57600         # RN2483 UART speed
CFLAGS += -DLORA_UART_BUFF_SIZE=255    # Buffer size for RX
CFLAGS += -DLORA_RST_PIN=14			   # RN2483 hardware reset pin

#
# I2C driver configuration
#
CFLAGS += -DI2C1_PINS=0
CFLAGS += -DI2C2_PINS=0
CFLAGS += -DI2C3_PINS=0
CFLAGS += -DI2C4_PINS=0
CFLAGS += -DI2C5_PINS=0

#
# Lua configuration
#
CFLAGS += -DDEBUG_FREE_MEM=1           # Enable LUA free mem debug utility (only for debug purposes)
CFLAGS += -DLUA_USE_LUA_LOCK=0		   # Enable if Lua must use real lua_lock / lua_unlock implementation
CFLAGS += -DLUA_USE_SAFE_SIGNAL=1      # Enable use of LuaOS safe signal (experimental)
CFLAGS += -DSTRCACHE_N=1
CFLAGS += -DSTRCACHE_M=1
CFLAGS += -DMINSTRTABSIZE=32
CFLAGS += -DLUA_USE_SHELL=0
CFLAGS += -DLUA_USE_EDITOR=0

#
# Standard Lua modules to include
#
CFLAGS += -DLUA_USE__G=1			   # base
CFLAGS += -DLUA_USE_OS=1			   # os
CFLAGS += -DLUA_USE_MATH=1		       # math
CFLAGS += -DLUA_USE_TABLE=1		       # table
CFLAGS += -DLUA_USE_IO=1		       # io
CFLAGS += -DLUA_USE_STRING=1		   # string
CFLAGS += -DLUA_USE_COROUTINE=1		   # coroutine
CFLAGS += -DLUA_USE_DEBUG=1			   # debug

#
# LuaOS Lua modules to include
#
CFLAGS += -DLUA_USE_TMR=1		       # timer
CFLAGS += -DLUA_USE_PIO=1		       # gpio
CFLAGS += -DLUA_USE_LORA=1		       # lora
CFLAGS += -DLUA_USE_PACK=1		       # pack
CFLAGS += -DLUA_USE_THREAD=1		   # thread
CFLAGS += -DLUA_USE_I2C=1			   # i2c