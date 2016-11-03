CFLAGS += -DPLATFORM_PIC32MZ
CFLAGS += -D__PIC32MZ__
CFLAGS += -mips32r2 -EL -mhard-float -fno-short-double -mfp64

#
# LuaOS configuration
# 
CFLAGS += -DCPU_HZ=200000000L                    # CPU frequency in hertz
CFLAGS += -DCORE_TIMER_HZ=CPU_HZ                 # CPU core timer frequency

CFLAGS += -DBASE_TIMER_HZ=1000                   # TIMER1 at 1000 Hz (T = 1 msec)
CFLAGS += -DBASE_TIMER_TICK_RATE=1000            # Generate a FreeRTOS ticks every 1000 TIMER1 ticks (T = 1 msec)

CFLAGS += -D_CLOCKS_PER_SEC_=configTICK_RATE_HZ  # Number of interrupt ticks for reach 1 second
CFLAGS += -DUSE_NETWORKING=0                	 # Networking is used (1 = yes, 0 = not)
CFLAGS += -DMTX_USE_EVENTS=0                 	 # Use event groups in mtx implementation (experimental)

CFLAGS += -DluaTaskStack=192*10              	 # Stck size assigned to lua thread
CFLAGS += -DtskDEF_PRIORITY=0U				 	 # Default task priority
CFLAGS += -DtskDEFStack=192*10				 	 # Default task priority
CFLAGS += -DdefaultThreadStack=192*10

CFLAGS += -DPBCLK2_HZ=100000000L 			 	 # System frequency for PMP/I2C/UART/SPI
CFLAGS += -DPBCLK3_HZ=40000000L 			 	 # System frequency for TIMERS
CFLAGS += -DPBCLK4_HZ=200000000L			 	 # System frequency for PORTS
CFLAGS += -DPBCLK5_HZ=40000000L 			 	 # System frequency for CAN

CFLAGS += -DUSE_RTC=0						 	 # Enable RTC
CFLAGS += -DLED_ACT=0x63					 	 # GPIO for activity led (0 if not led)
CFLAGS += -DLED_DBG=0x25					 	 # GPIO for debug led (0 if not led)

#
# SPI map
# 
CFLAGS += -DSPI1_PINS=0x4243    			 # sdi=RD2 ,sdo=RD3
CFLAGS += -DSPI1_CS=0x00
CFLAGS += -DSPI1_LED=0x00

CFLAGS += -DSPI2_PINS=0x7778    			 # sdi=RG7 ,sdo=RG8
CFLAGS += -DSPI2_CS=0x79      				 # cs=G9
CFLAGS += -DSPI2_LED=0x00

CFLAGS += -DSPI3_PINS=0x2a29    			 # sdi=RB10 ,sdo=RB9
CFLAGS += -DSPI3_CS=0x2b      				 # cs=RB11
CFLAGS += -DSPI3_LED=0x00

CFLAGS += -DSPI4_PINS=0x4b40    			 # sdi=RD11 ,sdo=RD0
CFLAGS += -DSPI4_CS=0x49      				 # cs=RD9
CFLAGS += -DSPI4_LED=0x00

#
# SDCARD configuration
# 
CFLAGS += -DUSE_SD=1                   # Enable SDCARD
CFLAGS += -DSD_SPI=1				   # SPI unit
CFLAGS += -DSD_CS=SPI1_CS			   # CS
CFLAGS += -DSD_LED=LED_ACT			   # LED

#
# CFI configuration
# 
CFLAGS += -DUSE_CFI=1                  # Enable CFI
CFLAGS += -DCFI_SPI=3				   # SPI unit
CFLAGS += -DCFI_CS=SPI3_CS  		   # CS
CFLAGS += -DCFI_LED=LED_ACT            # LED

#
# File system configuration
# 
CFLAGS += -DUSE_FAT=1                  # Enable FAT
CFLAGS += -DUSE_SPIFFS=0               # Enable SPIFFS

#
# Console configuration
#
CFLAGS += -DUSE_CONSOLE=1		       # Enable console
CFLAGS += -DCONSOLE_BR=115200	       # Console baud rate
CFLAGS += -DCONSOLE_UART=1		       # Console UART unit
CFLAGS += -DCONSOLE_SWAP_UART=0	       # Console alternative UART unit (0 = don't use alternative UART)
CFLAGS += -DCONSOLE_BUFFER_LEN=255     # Console buffer length in bytes

#
# LoraWAN driver connfiguration for RN2483
#
CFLAGS += -DUSE_LMIC=1
CFLAGS += -DUSE_RN2483=0

CFLAGS += -DUS_PER_OSTICK=16
CFLAGS += -DLMIC_TIMER_HZ=\(1000000/US_PER_OSTICK\)	   # 1 tick every 17us
CFLAGS += -DOSTICKS_PER_SEC=LMIC_TIMER_HZ

CFLAGS += -DLMIC_SPI=4				   # SPI unit
CFLAGS += -DLMIC_CS=SPI4_CS			   # CS
CFLAGS += -DLMIC_SPI_KHZ=10000
CFLAGS += -DLMIC_RST=0x50
CFLAGS += -DLMIC_DIO0=0x51
CFLAGS += -DLMIC_DIO1=0x52
CFLAGS += -DLMIC_DIO2=0x00

CFLAGS += -DLORA_UART=2				   # RN2483 UART unit
CFLAGS += -DLORA_UART_BR=57600         # RN2483 UART speed
CFLAGS += -DLORA_UART_BUFF_SIZE=255    # Buffer size for RX
CFLAGS += -DLORA_RST_PIN=0x50		   # RN2483 hardware reset pin

#
# I2C driver configuration
#
CFLAGS += -DI2C1_PINS=0x4a49		   # scl=D10, sda=D9
CFLAGS += -DI2C2_PINS=0
CFLAGS += -DI2C3_PINS=0
CFLAGS += -DI2C4_PINS=0x7877		   # scl=RG8, sda=RG7
CFLAGS += -DI2C5_PINS=0

#
# Lua configuration
#
CFLAGS += -DDEBUG_FREE_MEM=1           # Enable LUA free mem debug utility (only for debug purposes)
CFLAGS += -DLUA_USE_LUA_LOCK=0		   # Enable if Lua must use real lua_lock / lua_unlock implementation
CFLAGS += -DLUA_USE_SAFE_SIGNAL=0      # Enable use of LuaOS safe signal (experimental)
CFLAGS += -DSTRCACHE_N=1
CFLAGS += -DSTRCACHE_M=1
CFLAGS += -DMINSTRTABSIZE=32
CFLAGS += -DLUA_USE_HISTORY=1
CFLAGS += -DLUA_USE_SHELL=1
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
CFLAGS += -DLUA_USE_I2C=0			   # i2c
CFLAGS += -DLUA_USE_PWM=0			   # pwm
CFLAGS += -DLUA_USE_UART=0			   # uart