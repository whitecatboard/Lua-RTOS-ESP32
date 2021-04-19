#!/bin/bash

python -m pip uninstall kconfiglib -y # See https://github.com/espressif/esp-idf/issues/4066
cd /home/builder/Lua-RTOS-ESP32
source ./env # Use default env vars
IDF_PATH=~/esp-idf/ && PATH=$PATH:~/esp/xtensa-esp32-elf/bin/ #Override IDF and PATH
make SDKCONFIG_DEFAULTS=GENERIC defconfig # Default build for keep patches
make
