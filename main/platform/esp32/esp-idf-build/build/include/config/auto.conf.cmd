deps_config := \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/bt/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/esp32/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/freertos/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/log/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/lwip/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/mbedtls/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/spi_flash/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf-build/main/Kconfig.projbuild \
	/Users/jaumeolivepetrus/Lua-RTOS/main/platform/esp32/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
