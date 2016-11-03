deps_config := \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/log/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/spi_flash/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
