# What's LuaOS?

LuaOS is a real-time operating system designed to run on embedded systems, with minimal requirements of FLASH and RAM memory. Currently LuaOS is available for PIC32MZ and ESP8266 platforms, and can be easilly ported to other 32-bit platforms.

LuaOS is the main-core of the Whitecat ecosystem, that is being developed by a team of engineers, educators and living lab designers, designed for build Internet Of Things networks in an easy way.

LuaOS has a 3-layers design:

1. In the top layer there is a Lua 5.3.2 interpreter which offers to the programmer all resources provided by Lua 5.3.2 programming language, plus special modules for access the hardware (PIO, ADC, I2C, RTC, etc ...) and middleware services provided by LuaOS (LoRa WAN, MQTT, ...).

2. In the middle layer there is a Real-Time micro-kernel, powered by FreeRTOS. This is the responsible for that things happen in the expected time.

3. In the bottom layer there is a hardware abstraction layer, which talk directly with the platform hardware.

![](http://whitecatboard.org/git/luaos.png)

For porting LuaOS to other platforms is only necessary to write the code for the bottom layer, because the top and the middle layer are the same for all platforms.
