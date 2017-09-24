#!/bin/sh

source ./env
make SDKCONFIG_DEFAULTS=GENERIC defconfig
make flash monitor
