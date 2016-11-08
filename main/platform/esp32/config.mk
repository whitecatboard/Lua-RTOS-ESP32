CFLAGS += -DPLATFORM_ESP32

#
# LuaOS configuration
# 
CFLAGS += -DCPU_HZ=240000000L                   # CPU frequency in hertz
CFLAGS += -DCORE_TIMER_HZ=CPU_HZ                # CPU core timer frequency
CFLAGS += -D_CLOCKS_PER_SEC_=configTICK_RATE_HZ # Number of interrupt ticks for reach 1 second
CFLAGS += -DUSE_NETWORKING=0                    # Networking is used (1 = yes, 0 = not)
CFLAGS += -DMTX_USE_EVENTS=0                    # Use event groups in mtx implementation (experimental)

CFLAGS += -DluaTaskStack=1024*40                # Stck size assigned to lua thread
CFLAGS += -DtskDEFStack=192*10				    # Default task size
CFLAGS += -DtskDEF_PRIORITY=2				    # Default task priority
CFLAGS += -DdefaultThreadStack=10240
CFLAGS += -DPATH_MAX=64

CFLAGS += -DUSE_RTC=0						    # Enable RTC
CFLAGS += -DLED_ACT=16					 	    # GPIO for activity led (0 if not led)
CFLAGS += -DLED_DBG=0 					 	    # GPIO for debug led (0 if not led)

#
# File system configuration
# 
CFLAGS += -DUSE_FAT=1                  # Enable FAT
CFLAGS += -DUSE_SPIFFS=0               # Enable SPIFFS

#
# SPIFFS file system configuration
# 
CFLAGS += -DSPIFFS_ERASE_SIZE=4096     # SPI FLASH sector size (see your flah's datasheet)
CFLAGS += -DSPIFFS_LOG_PAGE_SIZE=256   # SPI FLASH page size (see your flah's datasheet)
CFLAGS += -DSPIFFS_LOG_BLOCK_SIZE=8192 # Logical block size, must be a miltiple of the page size
CFLAGS += -DSPIFFS_BASE_ADDR=0x180000  # SPI FLASH start adress for SPIFFS
CFLAGS += -DSPIFFS_SIZE=0x80000        # SPIFFS size

SPIFFS_ESPTOOL_ARGS = 0x180000 $(BUILD_DIR)spiffs_image.img

#
# Console configuration
#
CFLAGS += -DUSE_CONSOLE=1		       # Enable console
CFLAGS += -DCONSOLE_BR=115200	       # Console baud rate
CFLAGS += -DCONSOLE_UART=1		       # Console UART unit
CFLAGS += -DCONSOLE_SWAP_UART=0	       # Console alternative UART unit (0 = don't use alternative UART)
CFLAGS += -DCONSOLE_BUFFER_LEN=1024    # Console buffer length in bytes

#
# LoraWAN driver connfiguration for RN2483
#
CFLAGS += -DUSE_LMIC=1
CFLAGS += -DUSE_RN2483=0

# LMIC configuration
CFLAGS += -DUS_PER_OSTICK=20
CFLAGS += -DOSTICKS_PER_SEC=50000			
CFLAGS += -DLMIC_JOIN_FAILED_AFTER_868_864=1 # JOIN_FAILED after test 868 / 864, don't try lower DR

CFLAGS += -DLMIC_SPI=3				   # SPI unit
CFLAGS += -DLMIC_CS=5	  		       # CS
CFLAGS += -DLMIC_SPI_KHZ=10000
CFLAGS += -DLMIC_RST=27
CFLAGS += -DLMIC_DIO0=26
CFLAGS += -DLMIC_DIO1=25
CFLAGS += -DLMIC_DIO2=33

#
# SDCARD configuration
# 
CFLAGS += -DUSE_SD=1                   # Enable SDCARD
CFLAGS += -DSD_SPI=2				   # SPI unit
CFLAGS += -DSD_CS=15				   # CS
CFLAGS += -DSD_LED=LED_ACT			   # LED
CFLAGS += -DSD_KHZ=15000	           # Speed

#
# Display driver configuration
#
CFLAGS += -DDISPLAY_SPI=3
CFLAGS += -DDISPLAY_CS=5
CFLAGS += -DDISPLAY_RE=25
CFLAGS += -DDISPLAY_RS=26

# CFLAGS += -DLORA_UART=2			   # RN2483 UART unit
# CFLAGS += -DLORA_UART_BR=57600       # RN2483 UART speed
# CFLAGS += -DLORA_UART_BUFF_SIZE=255  # Buffer size for RX
# CFLAGS += -DLORA_RST_PIN=0x50		   # RN2483 hardware reset pin

#
# Lua configuration
#
CFLAGS += -DDEBUG_FREE_MEM=0           # Enable LUA free mem debug utility (only for debug purposes)
CFLAGS += -DLUA_USE_LUA_LOCK=0		   # Enable if Lua must use real lua_lock / lua_unlock implementation
CFLAGS += -DLUA_USE_SAFE_SIGNAL=0      # Enable use of LuaOS safe signal (experimental)
# CFLAGS += -DSTRCACHE_N=1
# CFLAGS += -DSTRCACHE_M=1
# CFLAGS += -DMINSTRTABSIZE=32
CFLAGS += -DLUA_USE_SHELL=1
CFLAGS += -DLUA_USE_EDITOR=1
CFLAGS += -DLUA_USE_HISTORY=1

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
CFLAGS += -DLUA_USE_DEBUG=1 		   # debug

#
# LuaOS Lua modules to include
#
CFLAGS += -DLUA_USE_TMR=1		       # timer
CFLAGS += -DLUA_USE_PIO=1		       # gpio
CFLAGS += -DLUA_USE_LORA=1		       # lora
CFLAGS += -DLUA_USE_PACK=1		       # pack
CFLAGS += -DLUA_USE_THREAD=1		   # thread
CFLAGS += -DLUA_USE_I2C=0   		   # i2c
CFLAGS += -DLUA_USE_SCREEN=1     	   # screen
CFLAGS += -DLUA_USE_NVS=1        	   # nvs
