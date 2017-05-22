# What's Lua RTOS?

Lua RTOS is a real-time operating system designed to run on embedded systems, with minimal requirements of FLASH and RAM memory. Currently Lua RTOS is available for ESP32, ESP8266 and PIC32MZ platforms, and can be easilly ported to other 32-bit platforms.

Lua RTOS has a 3-layer design:

1. In the top layer there is a Lua 5.3.4 interpreter which offers to the programmer all the resources provided by the Lua programming language, plus special modules for access the hardware (PIO, ADC, I2C, RTC, etc …), and middleware services provided by Lua RTOS (Lua Threads, LoRa WAN, MQTT, …).
1. In the middle layer there is a Real-Time micro-kernel, powered by FreeRTOS. This is the responsible for that things happen in the expected time.
1. In the bottom layer there is a hardware abstraction layer, which talk directly with the platform hardware.

![](http://git.whitecatboard.org/luartos.png)

For porting Lua RTOS to other platforms is only necessary to write the code for the bottom layer, because the top and the middle layer are the same for all platforms.

# How is it programmed?

The Lua RTOS compatible boards can be programmed with [The Whitecat IDE](https://ide.whitecatboard.org) in two ways: using the Lua programming language directly, or using a block-based programming language that translates blocks to Lua. No matter if you use Lua or blocks, both forms of programming are made from the same programming environment. The programmer can decide, for example, to made a fast prototype using blocks, then change to Lua, and finally back to blocks.

The Whitecat IDE is available at: [https://ide.whitecatboard.org](https://ide.whitecatboard.org).

![](http://git.whitecatboard.org/block-example.png)

![](http://git.whitecatboard.org/code-example.png)

In our [wiki](https://github.com/whitecatboard/Lua-RTOS-ESP32/wiki) you have more information about this.

# How to get Lua RTOS firmware?

## Prerequisites

1. Please note you need probably to download and install drivers for your board's USB-TO-SERIAL adapter for Windows and Mac OSX versions. The GNU/Linux version usually doesn't need any drivers. This drivers are required for connect to your board through a serial port connection.

   | Board              |
   |--------------------|
   | [WHITECAT ESP32 N1](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)  | 
   | [ESP32 CORE](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)  | 
   | [ESP32 THING](http://www.ftdichip.com/Drivers/VCP.htm)  | 

## Method 1: get a precompiled firmware

1. Install esptool (the ESP32 flasher utility), following  [this instructions](https://github.com/espressif/esptool).

1. Get the precompiled binaries for your board:

   | Board              |
   |--------------------|
   | [WHITECAT ESP32 N1](http://whitecatboard.org/firmware.php?board=WHITECAT-ESP32-N1)  | 
   | [ESP32 CORE](http://whitecatboard.org/firmware.php?board=ESP32-CORE-BOARD)  | 
   | [ESP32 THING](http://whitecatboard.org/firmware.php?board=ESP32-THING)  | 
   | [GENERIC](http://whitecatboard.org/firmware.php?board=GENERIC)  | 

1. Uncompress to your favorite folder:

   ```lua
   unzip LuaRTOS.10.WHITECAT-ESP32-N1.1488209955.zip
   ```

1. Go to the uncompressed folder, and flash:

   ```lua
   cd LuaRTOS.10.WHITECAT-ESP32-N1.1488209955
   ```

   For flash the firmware:
   
   ```lua
   python <esp-idf path>/components/esptool_py/esptool/esptool.py --chip esp32 --port "<usb path>" --baud 921600
   --before "default_reset" --after "hard_reset" write_flash -z --flash_mode "dio" --flash_freq "40m"
   --flash_size detect 0x1000 bootloader.WHITECAT-ESP32-N1.bin 0x10000
   lua_rtos.WHITECAT-ESP32-N1.bin 0x8000 partitions_singleapp.WHITECAT-ESP32-N1.bin
   ```

   For flash the filesystem:

   ```lua
   python <esp-idf path>/components/esptool_py/esptool/esptool.py --chip esp32 --port "<usb path>" --baud 921600
   --before "default_reset" --after "hard_reset" write_flash -z --flash_mode "dio" --flash_freq "40m"
   --flash_size detect 0x180000 spiffs_image.WHITECAT-ESP32-N1.bin
   ```
   
   Change "esp-idf path" and "usb path" according to your needs.

## Method 2: build by yourself

1. Install ESP32 toolchain for your desktop platform. Please, follow the instructions provided by ESPRESSIF:
   * [Windows](http://esp-idf.readthedocs.io/en/latest/get-started/windows-setup.html)
   * [Mac OS]( http://esp-idf.readthedocs.io/en/latest/get-started/macos-setup.html)
   * [Linux](http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html)

1. Clone esp-idf repository from ESPRESSIF:

   ```lua
   git clone --recursive https://github.com/espressif/esp-idf.git
   ```

1. Clone Lua RTOS repository:

   ```lua
   git clone --recursive https://github.com/whitecatboard/Lua-RTOS-ESP32
   ```
   
1. Setup the build environment:
   
   Go to Lua-RTOS-ESP32 folder:
   
   ```lua
   cd Lua-RTOS-ESP32
   ```
   
   Edit the env file and change PATH, IDF_PATH, LIBRARY_PATH, PKG_CONFIG_PATH, CPATH for fit to your installation locations.
   
   Now do:
   
   ```lua
   source ./env
   ```

1. Set the default configuration for your board:

   | Board              | Run this command                                     |
   |--------------------|------------------------------------------------------|
   | WHITECAT ESP32 N1  | make SDKCONFIG_DEFAULTS=WHITECAT-ESP32-N1 defconfig  |
   | ESP32 CORE         | make SDKCONFIG_DEFAULTS=ESP32-CORE-BOARD defconfig   |
   | ESP32 THING        | make SDKCONFIG_DEFAULTS=ESP32-THING defconfig        |
   | GENERIC            | make SDKCONFIG_DEFAULTS=GENERIC defconfig            |

1. Change the default configuration:

   You can change the default configuration doing:
   
   ```lua
   make menuconfig
   ```
   
   Remember to check the device name for your board's USB-TO-SERIAL adapter under the "Serial flasher config / Default serial port" category.
   
1. Compile:

   Build Lua RTOS, and flash it to your ESP32 board:

   ```lua
   make flash
   ```

   Flash the spiffs file system image to your ESP32 board:
   
   ```lua
   make flashfs
   ```
   
# Connect to the console

You can connect to the Lua RTOS console using your favorite terminal emulator program, such as picocom, minicom, hyperterminal, putty, etc ... The connection parameters are:

   * speed: 115200 bauds
   * data bits: 8
   * stop bits: 1
   * parity: none
   * terminal emulation: VT100

   For example, if you use picocom:
   
   ```lua
   picocom --baud 115200 /dev/tty.SLAB_USBtoUART
   ```
   
   ```lua
      /\       /\
     /  \_____/  \
   /_____________\
   W H I T E C A T

   Lua RTOS beta 0.1 build 1479953238 Copyright (C) 2015 - 2017 whitecatboard.org
   cpu ESP32 at 240 Mhz
   spiffs0 start address at 0x180000, size 512 Kb
   spiffs0 mounted
   spi2 at pins sdi=012/sdo=013/sck=014/cs=015
   sd0 is at spi2, pin cs=015
   sd0 type II, size 1943552 kbytes, speed 15 Mhz
   sd0a partition type 0b, sector 227, size 1943438 kbytes
   fat init file system
   fat0 mounted
   redirecting console messages to file system ...

   Lua RTOS beta 0.1 powered by Lua 5.3.4
   

   Executing /system.lua ...
   Executing /autorun.lua ...

   / > 
   ```

---
Lua RTOS is free for you, but funds are required for make it possible. Feel free to donate as little or as much as you wish. Every donation is very much appreciated.

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=M8BG7JGEPZUP6)
